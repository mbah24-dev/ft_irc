#!/usr/bin/env python3
"""
Suite de tests automatisés pour un serveur IRC (type ft_irc).

Exemples :
    python3 test_irc.py --port 6667 --password monmdp
    python3 test_irc.py --port 6667 --password monmdp --oper-pass adminpw --oper-user admin
    python3 test_irc.py --port 6667 --password monmdp --only MODE
    python3 test_irc.py --list

Résultats :
    PASS  : comportement conforme
    WARN  : comportement discutable / toléré (à vérifier par toi)
    FAIL  : comportement non conforme
    SKIP  : test non exécuté (ex. pas de --oper-pass)
"""
import argparse
import random
import re
import socket
import string
import sys
import threading
import time
import traceback
from collections import deque

# --------------------------------------------------------------------------- #
#  Utilitaires
# --------------------------------------------------------------------------- #

NUM_RE = re.compile(r"^(?::?\S+\s+)?(\d{3})(?:\s|$)")
BAD_PREFIX_RE = re.compile(r"^[^:\s]\S*\s+(\d{3})\s")      # "irc.srv 461 ..." sans ':'
EMPTY_TARGET_RE = re.compile(r"^:?\S+\s+(\d{3})\s\s")       # "461  PASS" : cible vide
EMPTY_PREFIX_RE = re.compile(r"^:!@")                           # ":!@host NICK x"
FORMAT_ISSUES = {}                                              # type -> exemples


def note_format(line):
    kind = None
    if BAD_PREFIX_RE.match(line):
        kind = "prefixe serveur sans ':' initial"
    elif EMPTY_TARGET_RE.match(line):
        kind = "numerique avec cible vide (attendu '*' avant enregistrement)"
    elif EMPTY_PREFIX_RE.match(line):
        kind = "prefixe utilisateur vide (':!@host')"
    if kind:
        ex = FORMAT_ISSUES.setdefault(kind, [])
        if len(ex) < 3 and line[:90] not in ex:
            ex.append(line[:90])


def numeric(line):
    m = NUM_RE.match(line)
    return int(m.group(1)) if m else None


def nums(lines):
    return [n for n in map(numeric, lines) if n is not None]


def rnd(n=5):
    return "".join(random.choices(string.ascii_lowercase, k=n))


def mode_str(lines, ch):
    """Extrait la chaîne de modes (ex: '+it') d'une réponse 324."""
    for l in lines:
        if numeric(l) == 324:
            p = l.split()
            if ch in p:
                i = p.index(ch)
                if i + 1 < len(p):
                    return p[i + 1].lstrip(":")
    return ""


class Warn(Exception):
    pass


class Skip(Exception):
    pass


def expect(cond, msg="assertion échouée"):
    if not cond:
        raise AssertionError(msg)


# --------------------------------------------------------------------------- #
#  Client IRC minimal
# --------------------------------------------------------------------------- #

class Client:
    def __init__(self, host, port):
        self.sock = socket.create_connection((host, port), timeout=3)
        self.buf = b""
        self.q = deque()
        self.closed = False
        self.nick = None

    def send(self, line, eol="\r\n"):
        self.raw((line + eol).encode("utf-8", "replace"))

    def raw(self, data):
        try:
            self.sock.sendall(data)
        except OSError:
            self.closed = True

    def _pump(self, timeout):
        if self.closed:
            return False
        self.sock.settimeout(max(timeout, 0.01))
        try:
            data = self.sock.recv(65536)
        except socket.timeout:
            return False
        except OSError:
            self.closed = True
            return False
        if not data:
            self.closed = True
            return False
        self.buf += data
        while b"\n" in self.buf:
            line, self.buf = self.buf.split(b"\n", 1)
            txt = line.rstrip(b"\r").decode("utf-8", "replace")
            note_format(txt)
            self.q.append(txt)
        return True

    def collect(self, quiet=0.3, maxtime=5):
        """Récupère tout ce qui arrive jusqu'à `quiet` secondes de silence."""
        out = []
        end = time.time() + maxtime
        while time.time() < end:
            while self.q:
                out.append(self.q.popleft())
            if not self._pump(quiet):
                break
        while self.q:
            out.append(self.q.popleft())
        return out

    def drain(self, quiet=0.2):
        return self.collect(quiet)

    def wait(self, pred, timeout=3):
        """Attend une ligne vérifiant pred. Retourne (trouvé, lignes lues)."""
        got = []
        end = time.time() + timeout
        while True:
            while self.q:
                l = self.q.popleft()
                got.append(l)
                if pred(l):
                    return True, got
            rem = end - time.time()
            if rem <= 0 or self.closed:
                return False, got
            self._pump(rem)

    def wait_num(self, *codes, timeout=3):
        return self.wait(lambda l: numeric(l) in codes, timeout)

    def wait_has(self, *subs, timeout=3):
        return self.wait(lambda l: all(s in l for s in subs), timeout)

    def wait_err(self, timeout=3):
        return self.wait(lambda l: (numeric(l) or 0) >= 400, timeout)

    def alive(self):
        tok = "alive" + rnd(4)
        self.send(f"PING {tok}")
        ok, _ = self.wait(lambda l: "PONG" in l and tok in l, 3)
        return ok

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


def need(c, *codes, timeout=3):
    ok, lines = c.wait_num(*codes, timeout=timeout)
    expect(ok, f"attendu {'/'.join(map(str, codes))}, reçu : {lines or 'rien'}")
    return lines


def need_has(c, *subs, timeout=3):
    ok, lines = c.wait_has(*subs, timeout=timeout)
    expect(ok, f"attendu une ligne contenant {subs}, reçu : {lines or 'rien'}")
    return lines


# --------------------------------------------------------------------------- #
#  Contexte de test
# --------------------------------------------------------------------------- #

class Ctx:
    def __init__(self, args):
        self.a = args
        self.clients = []

    def raw(self):
        c = Client(self.a.host, self.a.port)
        self.clients.append(c)
        return c

    def nick(self):
        return "u" + rnd(6)

    def chan(self):
        return "#c" + rnd(5)

    def register_raw(self, c, nick, user=None, eol="\r\n"):
        c.nick = nick
        user = user or nick
        c.send(f"PASS {self.a.password}", eol)
        c.send(f"NICK {nick}", eol)
        c.send(f"USER {user} 0 * :Test {nick}", eol)

    def client(self, nick=None, user=None, register=True):
        c = self.raw()
        c.nick = nick or self.nick()
        if register:
            self.register_raw(c, c.nick, user)
            ok, lines = c.wait_num(1, timeout=4)
            expect(ok, f"enregistrement : pas de 001 pour {c.nick} (reçu {lines})")
            c.drain(0.2)
        return c

    def join(self, c, ch, key=None):
        c.send(f"JOIN {ch}" + (f" {key}" if key else ""))
        _, lines = c.wait(lambda l: numeric(l) == 366 or (numeric(l) or 0) >= 400, 3)
        return lines

    def group(self, n=2, ch=None):
        """n clients dans un même channel (le premier est opérateur)."""
        ch = ch or self.chan()
        cs = []
        for _ in range(n):
            c = self.client()
            lines = self.join(c, ch)
            expect(366 in nums(lines), f"{c.nick} n'a pas pu rejoindre {ch} : {lines}")
            cs.append(c)
        for c in cs:
            c.drain(0.15)
        return cs, ch

    def chan_in_list(self, c, ch):
        c.send("LIST")
        _, lines = c.wait_num(323)
        return any(numeric(l) == 322 and ch in l.split() for l in lines)

    def server_alive(self):
        try:
            self.client()
            return True
        except Exception:
            return False

    def close_all(self):
        for c in self.clients:
            c.close()
        self.clients = []


TESTS = []


def test(section, name):
    def deco(fn):
        TESTS.append((section, name, fn))
        return fn
    return deco


# --------------------------------------------------------------------------- #
#  1. Enregistrement
# --------------------------------------------------------------------------- #

S = "1.Enregistrement"


@test(S, "PASS sans paramètre -> 461")
def _(x):
    c = x.raw(); c.send("PASS"); need(c, 461)


@test(S, "PASS correct -> message client")
def _(x):
    c = x.raw(); c.send(f"PASS {x.a.password}"); need_has(c, "Password accepted")


@test(S, "PASS incorrect -> 464")
def _(x):
    c = x.raw(); c.send(f"PASS {x.a.password}_faux"); need(c, 464)


@test(S, "PASS après enregistrement -> 462")
def _(x):
    c = x.client(); c.send(f"PASS {x.a.password}"); need(c, 462)


@test(S, "NICK sans PASS -> 464")
def _(x):
    c = x.raw(); c.send("NICK abcdef"); need(c, 464)


@test(S, "NICK sans paramètre -> 431")
def _(x):
    c = x.raw(); c.send(f"PASS {x.a.password}"); c.send("NICK"); need(c, 431)


@test(S, "NICK invalides -> 432")
def _(x):
    c = x.raw(); c.send(f"PASS {x.a.password}")
    for bad in ["#chan", "1abc", "a,b", "@x", "-abc"]:
        c.send(f"NICK {bad}")
        ok, l = c.wait_num(432)
        expect(ok, f"NICK {bad} : attendu 432, reçu {l or 'rien'}")


@test(S, "NICK déjà pris -> 433")
def _(x):
    a = x.client(); b = x.raw()
    b.send(f"PASS {x.a.password}"); b.send(f"NICK {a.nick}"); need(b, 433)


@test(S, "USER avec < 3 paramètres -> 461")
def _(x):
    c = x.raw(); c.send(f"PASS {x.a.password}"); c.send(f"NICK {x.nick()}")
    c.send("USER a"); need(c, 461)


@test(S, "USER après enregistrement -> 462")
def _(x):
    c = x.client(); c.send("USER a 0 * :x"); need(c, 462)


@test(S, "Enregistrement complet -> 001")
def _(x):
    c = x.raw(); x.register_raw(c, x.nick())
    lines = need(c, 1)
    rest = c.collect(0.4)
    print(f"        (numériques reçus : {sorted(set(nums(lines + rest)))})")


@test(S, "Changement de NICK propagé aux channels communs")
def _(x):
    (a, b), ch = x.group(2)
    old, new = a.nick, x.nick()
    a.send(f"NICK {new}")
    need_has(b, "NICK", new)
    a.nick = new


@test(S, "NICK vers son propre pseudo : ni 433 ni crash")
def _(x):
    a = x.client(); a.send(f"NICK {a.nick}")
    lines = a.collect(0.5)
    expect(433 not in nums(lines), "433 renvoyé pour son propre pseudo")
    expect(a.alive(), "le client ne répond plus")


@test(S, "Commandes avant enregistrement -> 451")
def _(x):
    c = x.raw()
    c.send("JOIN #a"); need(c, 451)
    c.send("PRIVMSG x :y"); need(c, 451)
    d = x.raw(); d.send(f"PASS {x.a.password}"); d.send(f"NICK {x.nick()}")
    d.send("JOIN #a"); need(d, 451)


@test(S, "CAP LS / CAP END puis enregistrement")
def _(x):
    c = x.raw(); c.send("CAP LS")
    ok, l = c.wait_has("CAP")
    expect(ok, f"pas de réponse à CAP LS : {l}")
    c.send("CAP END"); x.register_raw(c, x.nick()); need(c, 1, timeout=4)


@test(S, "CAP REQ : réponse ACK/NAK sans blocage")
def _(x):
    c = x.raw(); c.send("CAP REQ :multi-prefix")
    ok, l = c.wait(lambda l: "CAP" in l and ("ACK" in l or "NAK" in l))
    c.send("CAP END"); x.register_raw(c, x.nick())
    need(c, 1, timeout=4)
    if not ok:
        raise Warn("pas de ACK/NAK à CAP REQ (enregistrement OK malgré tout)")


# --------------------------------------------------------------------------- #
#  2. PING / PONG
# --------------------------------------------------------------------------- #

S = "2.PING/PONG"


@test(S, "PING token -> PONG contenant le token")
def _(x):
    c = x.client(); c.send("PING test123")
    lines = need_has(c, "PONG", "test123")
    print(f"        (réponse : {lines[-1]})")


@test(S, "PING sans paramètre -> 461/409")
def _(x):
    c = x.client(); c.send("PING"); need(c, 461, 409)


@test(S, "PONG token : aucune erreur")
def _(x):
    c = x.client(); c.send("PONG test123")
    lines = c.collect(0.5)
    expect(not any(n >= 400 for n in nums(lines)), f"erreur reçue : {lines}")
    expect(c.alive(), "client mort")


@test(S, "Keepalive (option --keepalive SECONDES)")
def _(x):
    if not x.a.keepalive:
        raise Skip("utilise --keepalive 300 pour tester 5 minutes")
    c = x.client(); end = time.time() + x.a.keepalive
    while time.time() < end and not c.closed:
        ok, l = c.wait(lambda l: l.startswith("PING"), 2)
        if ok:
            c.send("PONG " + l[-1].split(" ", 1)[1])
    expect(not c.closed, "déconnecté à tort pendant l'inactivité")
    expect(c.alive(), "ne répond plus après l'inactivité")


# --------------------------------------------------------------------------- #
#  3. PRIVMSG / NOTICE
# --------------------------------------------------------------------------- #

S = "3.PRIVMSG/NOTICE"


@test(S, "PRIVMSG user -> user, préfixe nick!user@host")
def _(x):
    a, b = x.client(), x.client()
    a.send(f"PRIVMSG {b.nick} :salut tout le monde")
    l = need_has(b, "PRIVMSG", "salut tout le monde")[-1]
    expect(l.startswith(f":{a.nick}!") and "@" in l.split()[0], f"préfixe incorrect : {l}")


@test(S, "PRIVMSG sans cible -> 411, sans texte -> 412, texte vide -> 412")
def _(x):
    a, b = x.client(), x.client()
    a.send("PRIVMSG"); need(a, 411)
    a.send(f"PRIVMSG {b.nick}"); need(a, 412)
    a.send(f"PRIVMSG {b.nick} :"); need(a, 412)


@test(S, "PRIVMSG cible inconnue -> 401")
def _(x):
    a = x.client(); a.send("PRIVMSG nobody" + rnd() + " :x"); need(a, 401)


@test(S, "PRIVMSG #chan sans être membre -> 404")
def _(x):
    (a,), ch = x.group(1); b = x.client()
    b.send(f"PRIVMSG {ch} :x"); need(b, 404, 442)


@test(S, "PRIVMSG #chan : reçu par tous sauf l'émetteur")
def _(x):
    (a, b, c), ch = x.group(3)
    a.send(f"PRIVMSG {ch} :hello chan")
    need_has(b, "PRIVMSG", ch, "hello chan")
    need_has(c, "PRIVMSG", ch, "hello chan")
    expect(not any("hello chan" in l for l in a.collect(0.5)), "l'émetteur a reçu son propre message")


@test(S, "PRIVMSG #inexistant -> 403/401/404")
def _(x):
    a = x.client(); a.send("PRIVMSG #nope" + rnd() + " :x"); need(a, 403, 401, 404)


@test(S, "Texte spécial : accents, ':', espaces multiples, UTF-8")
def _(x):
    a, b = x.client(), x.client()
    txt = "héllo wörld : avec  deux   espaces 日本語 ✓"
    a.send(f"PRIVMSG {b.nick} :{txt}")
    need_has(b, txt)


@test(S, "Message très long (600 octets) : pas de crash")
def _(x):
    a, b = x.client(), x.client()
    a.send(f"PRIVMSG {b.nick} :" + "Z" * 600)
    b.collect(0.5)
    expect(x.server_alive(), "serveur mort après message long")


@test(S, "PRIVMSG multi-cibles 'a,b' : géré ou refusé proprement")
def _(x):
    a, b, c = x.client(), x.client(), x.client()
    a.send(f"PRIVMSG {b.nick},{c.nick} :multi")
    got_b, got_c, got_a = b.collect(0.5), c.collect(0.3), a.collect(0.3)
    ok = any("multi" in l for l in got_b) and any("multi" in l for l in got_c)
    expect(ok or any(n >= 400 for n in nums(got_a)), "ni livré aux 2 cibles, ni erreur")
    if not ok:
        raise Warn("multi-cibles non supporté (erreur renvoyée) : acceptable")


@test(S, "NOTICE user/channel livrés")
def _(x):
    (a, b), ch = x.group(2)
    a.send(f"NOTICE {b.nick} :psst"); need_has(b, "NOTICE", "psst")
    a.send(f"NOTICE {ch} :bonjour"); need_has(b, "NOTICE", ch, "bonjour")


@test(S, "NOTICE : jamais d'erreur (cible inconnue, sans texte, sans param)")
def _(x):
    a = x.client()
    a.send("NOTICE nobody" + rnd() + " :x"); a.send("NOTICE"); a.send("NOTICE #nope :x")
    a.send(f"NOTICE {a.nick}")
    lines = a.collect(0.6)
    expect(not any(n >= 400 for n in nums(lines)), f"NOTICE a généré une erreur : {lines}")


@test(S, "NOTICE avant enregistrement : silence, pas de crash")
def _(x):
    c = x.raw(); c.send("NOTICE x :y")
    c.collect(0.5)
    expect(x.server_alive(), "serveur mort")


# --------------------------------------------------------------------------- #
#  4. JOIN / PART / TOPIC / LIST / NAMES / WHO
# --------------------------------------------------------------------------- #

S = "4.Channels"


@test(S, "JOIN crée le channel : JOIN + 353 avec @ + 366")
def _(x):
    a = x.client(); ch = x.chan()
    lines = x.join(a, ch)
    n = nums(lines)
    expect(any("JOIN" in l and ch in l for l in lines), f"pas de JOIN écho : {lines}")
    expect(353 in n and 366 in n, f"353/366 manquants : {lines}")
    expect(any(numeric(l) == 353 and "@" + a.nick in l for l in lines), "créateur pas opérateur (@)")
    print(f"        (topic : {'332' if 332 in n else '331' if 331 in n else 'aucun 331/332'})")


@test(S, "JOIN sans '#' -> 479/403")
def _(x):
    a = x.client(); a.send("JOIN general"); need(a, 479, 403)


@test(S, "JOIN sans paramètre -> 461")
def _(x):
    a = x.client(); a.send("JOIN"); need(a, 461)


@test(S, "JOIN alors qu'on est déjà dedans -> 443 (ou ignoré)")
def _(x):
    (a,), ch = x.group(1)
    a.send(f"JOIN {ch}")
    lines = a.collect(0.5)
    expect(a.alive(), "client mort")
    if 443 not in nums(lines):
        raise Warn("pas de 443 (ignoré silencieusement ?)")


@test(S, "Un nouvel arrivant est annoncé aux membres")
def _(x):
    (a,), ch = x.group(1)
    b = x.client(); b.send(f"JOIN {ch}")
    need_has(a, "JOIN", ch, b.nick)


@test(S, "JOIN multiples '#a,#b' et clés 'k1,k2'")
def _(x):
    a, b = x.client(), x.client()
    c1, c2 = x.chan(), x.chan()
    x.join(a, c1); x.join(a, c2)
    a.send(f"MODE {c1} +k s1"); a.send(f"MODE {c2} +k s2"); a.drain(0.3)
    b.send(f"JOIN {c1},{c2} s1,s2")
    ok1, _ = b.wait(lambda l: numeric(l) == 366 and c1 in l)
    ok2, _ = b.wait(lambda l: numeric(l) == 366 and c2 in l)
    expect(ok1 and ok2, "JOIN multiple avec clés non géré")


@test(S, "PART diffusé aux membres avec la raison")
def _(x):
    (a, b), ch = x.group(2)
    b.send(f"PART {ch} :bye bye")
    need_has(a, "PART", ch, "bye bye")


@test(S, "PART #inexistant -> 403 ; PART non membre -> 442")
def _(x):
    (a,), ch = x.group(1); b = x.client()
    b.send("PART #nope" + rnd()); need(b, 403)
    b.send(f"PART {ch}"); need(b, 442)


@test(S, "Dernier membre part -> channel supprimé (LIST)")
def _(x):
    (a,), ch = x.group(1); b = x.client()
    a.send(f"PART {ch}"); a.drain(0.3)
    expect(not x.chan_in_list(b, ch), "le channel est toujours listé")


@test(S, "Channel recréé : nouvel opérateur, sans ancien topic/modes")
def _(x):
    (a,), ch = x.group(1); b = x.client()
    a.send(f"TOPIC {ch} :ancien"); a.send(f"MODE {ch} +tk cle"); a.drain(0.3)
    a.send(f"PART {ch}"); a.drain(0.3)
    lines = x.join(b, ch)
    expect(366 in nums(lines), f"JOIN impossible (anciens modes conservés ?) : {lines}")
    expect(any(numeric(l) == 353 and "@" + b.nick in l for l in lines), "pas opérateur")
    expect(not any(numeric(l) == 332 and "ancien" in l for l in lines), "ancien topic conservé")


@test(S, "TOPIC : 331 puis set diffusé puis 332")
def _(x):
    (a, b), ch = x.group(2)
    a.send(f"TOPIC {ch}"); need(a, 331)
    a.send(f"TOPIC {ch} :nouveau sujet")
    need_has(b, "TOPIC", ch, "nouveau sujet")
    a.send(f"TOPIC {ch}"); need_has(a, "332", "nouveau sujet")


@test(S, "TOPIC avec +t : non-op -> 482 ; sans +t : autorisé")
def _(x):
    (a, b), ch = x.group(2)
    b.send(f"TOPIC {ch} :libre"); need_has(a, "TOPIC", "libre")
    a.send(f"MODE {ch} +t"); a.drain(0.3); b.drain(0.3)
    b.send(f"TOPIC {ch} :interdit"); need(b, 482)


@test(S, "TOPIC depuis un non-membre -> 442")
def _(x):
    (a,), ch = x.group(1); c = x.client()
    c.send(f"TOPIC {ch}"); need(c, 442)


@test(S, "LIST : 321, 322 (avec nb membres), 323")
def _(x):
    (a, b), ch = x.group(2); c = x.client()
    c.send("LIST")
    ok, lines = c.wait_num(323)
    n = nums(lines)
    expect(321 in n and ok, f"321/323 manquants : {lines}")
    row = [l for l in lines if numeric(l) == 322 and ch in l.split()]
    expect(row, "channel absent de la liste")
    p = row[0].split()
    expect(p[p.index(ch) + 1] == "2", f"nombre de membres incorrect : {row[0]}")
    c.send(f"LIST {ch}"); need(c, 323)


@test(S, "NAMES : préfixe @ pour les opérateurs, 366")
def _(x):
    (a, b), ch = x.group(2)
    a.send(f"NAMES {ch}")
    lines = need(a, 366)
    l353 = " ".join(l for l in lines if numeric(l) == 353)
    expect("@" + a.nick in l353 and b.nick in l353 and "@" + b.nick not in l353,
           f"353 incorrect : {l353}")
    a.send("NAMES"); need(a, 366)


@test(S, "WHO : channel / nick / * / 'o' / inexistant -> 315")
def _(x):
    (a, b), ch = x.group(2)
    a.send(f"WHO {ch}")
    l = need(a, 315)
    expect(sum(numeric(z) == 352 for z in l) >= 2, f"moins de 2 lignes 352 : {l}")
    a.send(f"WHO {b.nick}")
    l = need(a, 315); expect(any(numeric(z) == 352 and b.nick in z for z in l), "352 manquant pour le nick")
    a.send("WHO *"); need(a, 315)
    a.send(f"WHO {ch} o")
    l = need(a, 315)
    l352 = [z for z in l if numeric(z) == 352]
    expect(any(a.nick in z for z in l352) and not any(b.nick in z for z in l352), "filtre 'o' incorrect")
    a.send("WHO nobody" + rnd()); need(a, 315)


# --------------------------------------------------------------------------- #
#  5. MODE / INVITE / KICK
# --------------------------------------------------------------------------- #

S = "5.MODE/INVITE/KICK"


@test(S, "MODE #chan -> 324")
def _(x):
    (a,), ch = x.group(1); a.send(f"MODE {ch}"); need(a, 324)


@test(S, "MODE +i par un non-op -> 482")
def _(x):
    (a, b), ch = x.group(2); b.send(f"MODE {ch} +i"); need(b, 482)


@test(S, "+i : JOIN refusé (473), INVITE puis JOIN OK, invitation consommée")
def _(x):
    (a,), ch = x.group(1); c = x.client()
    a.send(f"MODE {ch} +i"); a.drain(0.3)
    c.send(f"JOIN {ch}"); need(c, 473)
    a.send(f"INVITE {c.nick} {ch}"); need(a, 341)
    need_has(c, "INVITE", ch)
    expect(366 in nums(x.join(c, ch)), "JOIN refusé malgré l'invitation")
    c.send(f"PART {ch}"); c.drain(0.3)
    c.send(f"JOIN {ch}"); need(c, 473)


@test(S, "+k : mauvaise clé -> 475, bonne clé OK, -k retire")
def _(x):
    (a,), ch = x.group(1); c, d = x.client(), x.client()
    a.send(f"MODE {ch} +k secret"); a.drain(0.3)
    c.send(f"JOIN {ch}"); need(c, 475)
    c.send(f"JOIN {ch} mauvaise"); need(c, 475)
    expect(366 in nums(x.join(c, ch, "secret")), "bonne clé refusée")
    a.send(f"MODE {ch} -k secret"); a.drain(0.3)
    expect(366 in nums(x.join(d, ch)), "JOIN refusé après -k")


@test(S, "+l 2 : 3e client -> 471 ; valeurs invalides sans crash ; +l seul -> 461")
def _(x):
    (a, b), ch = x.group(2); c = x.client()
    a.send(f"MODE {ch} +l 2"); a.drain(0.3)
    c.send(f"JOIN {ch}"); need(c, 471)
    for bad in ("+l 0", "+l -1", "+l abc"):
        a.send(f"MODE {ch} {bad}"); a.collect(0.3)
    a.send(f"MODE {ch} +l"); need(a, 461)
    expect(a.alive(), "client mort après +l invalides")


@test(S, "+o / -o : diffusé, visible dans NAMES")
def _(x):
    (a, b), ch = x.group(2)
    a.send(f"MODE {ch} +o {b.nick}")
    need_has(b, "MODE", ch, "+o", b.nick)
    a.send(f"NAMES {ch}")
    expect("@" + b.nick in " ".join(need(a, 366)), "@ absent après +o")
    a.send(f"MODE {ch} -o {b.nick}"); a.drain(0.3)
    a.send(f"NAMES {ch}")
    expect("@" + b.nick not in " ".join(need(a, 366)), "@ encore présent après -o")


@test(S, "+o sur pseudo inconnu -> 401/441")
def _(x):
    (a,), ch = x.group(1); a.send(f"MODE {ch} +o nobody{rnd()}"); need(a, 401, 441)


@test(S, "Mode inconnu +z -> 472")
def _(x):
    (a,), ch = x.group(1); a.send(f"MODE {ch} +z"); need(a, 472)


@test(S, "Modes combinés : +it, +kl secret 5, +i-t, -kl secret")
def _(x):
    (a,), ch = x.group(1)

    def q():
        a.send(f"MODE {ch}")
        return mode_str(need(a, 324), ch)

    a.send(f"MODE {ch} +it"); a.drain(0.3)
    m = q(); expect("i" in m and "t" in m, f"+it : modes = '{m}'")
    a.send(f"MODE {ch} +kl secret 5"); a.drain(0.3)
    m = q(); expect("k" in m and "l" in m, f"+kl : modes = '{m}'")
    a.send(f"MODE {ch} +i-t"); a.drain(0.3)
    m = q(); expect("i" in m and "t" not in m, f"+i-t : modes = '{m}'")
    a.send(f"MODE {ch} -kl secret"); a.drain(0.3)
    m = q(); expect("k" not in m and "l" not in m, f"-kl : modes = '{m}'")


@test(S, "Changement de mode diffusé à tous les membres")
def _(x):
    (a, b, c), ch = x.group(3)
    a.send(f"MODE {ch} +t")
    need_has(b, "MODE", ch, "+t"); need_has(c, "MODE", ch, "+t")


@test(S, "Un op se retire ses droits : pas de crash")
def _(x):
    (a, b), ch = x.group(2)
    a.send(f"MODE {ch} -o {a.nick}"); a.collect(0.4)
    expect(a.alive() and x.server_alive(), "crash")


@test(S, "INVITE : 341, cible reçoit INVITE")
def _(x):
    (a,), ch = x.group(1); c = x.client()
    a.send(f"INVITE {c.nick} {ch}"); need(a, 341); need_has(c, "INVITE", ch)


@test(S, "INVITE : 401 nick inconnu, 403 channel inexistant, 442, 443")
def _(x):
    (a, b), ch = x.group(2); c = x.client()
    a.send(f"INVITE nobody{rnd()} {ch}"); need(a, 401)
    a.send(f"INVITE {c.nick} #nope{rnd()}"); need(a, 403, 401)
    c.send(f"INVITE {a.nick} {ch}"); need(c, 442)
    a.send(f"INVITE {b.nick} {ch}"); need(a, 443)


@test(S, "INVITE en +i par un non-op -> 482")
def _(x):
    (a, b), ch = x.group(2); c = x.client()
    a.send(f"MODE {ch} +i"); a.drain(0.3)
    b.send(f"INVITE {c.nick} {ch}"); need(b, 482)


@test(S, "KICK diffusé (y compris à la cible), cible retirée")
def _(x):
    (a, b, c), ch = x.group(3)
    a.send(f"KICK {ch} {b.nick} :raison")
    for cl in (a, b, c):
        need_has(cl, "KICK", ch, b.nick, "raison")
    c.send(f"PRIVMSG {ch} :apres kick"); c.drain(0.2)
    expect(not any("apres kick" in l for l in b.collect(0.5)), "b reçoit encore les messages")


@test(S, "KICK : non-op 482, cible absente 441, channel inexistant 403")
def _(x):
    (a, b, c), ch = x.group(3); d = x.client()
    b.send(f"KICK {ch} {c.nick}"); need(b, 482)
    a.send(f"KICK {ch} {d.nick}"); need(a, 441)
    a.send(f"KICK #nope{rnd()} {b.nick}"); need(a, 403)


@test(S, "Auto-kick (dernier membre) : pas de crash, channel supprimé")
def _(x):
    (a,), ch = x.group(1); b = x.client()
    a.send(f"KICK {ch} {a.nick} :moi"); a.collect(0.5)
    expect(x.server_alive(), "crash")
    expect(not x.chan_in_list(b, ch), "channel encore listé")


# --------------------------------------------------------------------------- #
#  6. Commandes opérateur
# --------------------------------------------------------------------------- #

S = "6.Operateur"


def need_oper(x):
    if not x.a.oper_pass:
        raise Skip("utilise --oper-pass (et --oper-user) pour tester")


def make_oper(x):
    o = x.client(user=x.a.oper_user)
    o.send(f"OPER {x.a.oper_user} {x.a.oper_pass}")
    need(o, 381)
    o.drain(0.2)
    return o


@test(S, "OPER : sans param 461, mauvais user/mdp refusés, bon -> 381")
def _(x):
    need_oper(x)
    c = x.client(user=x.a.oper_user)
    c.send("OPER"); need(c, 461)
    c.send(f"OPER autreuser {x.a.oper_pass}")
    ok, l = c.wait_err(); expect(ok, f"OPER avec mauvais user : erreur attendue, reçu {l or 'rien'}")
    c.send(f"OPER {x.a.oper_user} mauvaismdp"); need(c, 464)
    c.send(f"OPER {x.a.oper_user} {x.a.oper_pass}"); need(c, 381)


@test(S, "OPER avec un username différent du USER -> refusé")
def _(x):
    need_oper(x)
    c = x.client(user="autre" + rnd(3))
    c.send(f"OPER {x.a.oper_user} {x.a.oper_pass}")
    ok, l = c.wait(lambda l: numeric(l) in (381,) or (numeric(l) or 0) >= 400)
    expect(ok and 381 not in nums(l), f"OPER accepté à tort ou pas de réponse : {l}")


@test(S, "KILL/GLOBOPS par un non-op -> 481")
def _(x):
    a, b = x.client(), x.client()
    a.send(f"KILL {b.nick} :x"); need(a, 481)
    a.send("GLOBOPS test"); need(a, 481)


@test(S, "KILL : inconnu -> 401 ; cible déconnectée et channels notifiés")
def _(x):
    need_oper(x)
    o = make_oper(x)
    o.send("KILL nobody" + rnd() + " :x"); need(o, 401)
    (b, c), ch = x.group(2)
    o.send(f"KILL {b.nick} :bye")
    b.collect(1.0)
    expect(b.closed, "la cible n'a pas été déconnectée")
    need_has(c, b.nick)


@test(S, "KILL sur soi-même : pas de crash")
def _(x):
    need_oper(x)
    o = make_oper(x); o.send(f"KILL {o.nick} :suicide"); o.collect(0.6)
    expect(x.server_alive(), "serveur mort")


@test(S, "GLOBOPS : reçu uniquement par les opérateurs")
def _(x):
    need_oper(x)
    o1, o2, n = make_oper(x), make_oper(x), x.client()
    o1.send("GLOBOPS message-secret-ops")
    need_has(o2, "message-secret-ops")
    expect(not any("message-secret-ops" in l for l in n.collect(0.6)), "un non-op l'a reçu")


@test(S, "SHOWTIME -> 391 ; param en trop -> erreur ; avant enreg. -> 451")
def _(x):
    a = x.client(); a.send("SHOWTIME"); need(a, 391)
    a.send("SHOWTIME abc")
    ok, l = a.wait_err(2)
    expect(ok, f"SHOWTIME abc : erreur attendue, reçu {l or 'rien'}")
    c = x.raw(); c.send("SHOWTIME"); need(c, 451)


# --------------------------------------------------------------------------- #
#  7. QUIT / déconnexions
# --------------------------------------------------------------------------- #

S = "7.QUIT"


@test(S, "QUIT :raison propagé avec préfixe complet")
def _(x):
    (a, b), ch = x.group(2)
    a.send("QUIT :je pars")
    l = need_has(b, "QUIT", "je pars")[-1]
    expect(l.startswith(f":{a.nick}!"), f"préfixe incorrect : {l}")


@test(S, "QUIT sans raison")
def _(x):
    (a, b), ch = x.group(2); a.send("QUIT"); need_has(b, "QUIT", a.nick)


@test(S, "Déconnexion brutale : QUIT diffusé, pseudo libéré")
def _(x):
    (a, b), ch = x.group(2); nick = a.nick
    a.close()
    need_has(b, "QUIT", nick, timeout=4)
    x.client(nick=nick)


@test(S, "Dernier membre disparaît : channel supprimé")
def _(x):
    (a,), ch = x.group(1); b = x.client()
    a.close(); time.sleep(0.5)
    expect(not x.chan_in_list(b, ch), "channel encore listé")


@test(S, "L'op du channel se déconnecte : pas de crash")
def _(x):
    (a, b), ch = x.group(2)
    a.close(); b.collect(0.5)
    b.send(f"NAMES {ch}"); need(b, 366)
    b.send(f"PRIVMSG {ch} :toujours la"); expect(b.alive(), "client mort")


@test(S, "Déconnexion pendant l'enregistrement (après PASS / après NICK)")
def _(x):
    c = x.raw(); c.send(f"PASS {x.a.password}"); c.close()
    n = x.nick()
    d = x.raw(); d.send(f"PASS {x.a.password}"); d.send(f"NICK {n}"); d.close()
    time.sleep(0.4)
    x.client(nick=n)


# --------------------------------------------------------------------------- #
#  8. Robustesse
# --------------------------------------------------------------------------- #

S = "8.Robustesse"


@test(S, "Commandes fragmentées (3 octets à la fois)")
def _(x):
    c = x.raw(); n = x.nick()
    data = f"PASS {x.a.password}\r\nNICK {n}\r\nUSER {n} 0 * :Frag\r\n".encode()
    for i in range(0, len(data), 3):
        c.raw(data[i:i + 3]); time.sleep(0.02)
    need(c, 1, timeout=4)


@test(S, "Commandes groupées dans un seul send()")
def _(x):
    c = x.raw(); n = x.nick(); ch = x.chan()
    c.raw((f"PASS {x.a.password}\r\nNICK {n}\r\nUSER {n} 0 * :G\r\n"
           f"JOIN {ch}\r\nPRIVMSG {ch} :hi\r\nPING grp\r\n").encode())
    l = need(c, 366, timeout=4)
    need_has(c, "PONG", "grp")


@test(S, "Fin de ligne '\\n' seul")
def _(x):
    c = x.raw(); x.register_raw(c, x.nick(), eol="\n")
    ok, l = c.wait_num(1, timeout=3)
    if not ok:
        raise Warn("\\n seul non toléré (les vrais clients envoient \\r\\n)")


@test(S, "Commandes insensibles à la casse")
def _(x):
    a = x.client(); ch = x.chan()
    a.send(f"join {ch}")
    _, l = a.wait_num(366, 421)
    ok_join = 366 in nums(l)
    a.send("pInG zz")
    ok_ping, _ = a.wait(lambda l: "PONG" in l and "zz" in l, 2)
    if not (ok_join and ok_ping):
        raise Warn("commandes sensibles a la casse (les vrais clients envoient des majuscules)")


@test(S, "Casse des pseudos / channels (RFC : insensible)")
def _(x):
    a = x.client(nick="Al" + rnd(4)); b = x.raw()
    b.send(f"PASS {x.a.password}"); b.send(f"NICK {a.nick.lower()}")
    ok, _ = b.wait_num(433, timeout=1.5)
    c1 = x.chan()
    a2, b2 = x.client(), x.client()
    x.join(a2, "#C" + c1[2:].upper()); x.join(b2, c1)
    a2.drain(0.3)
    a2.send(f"NAMES #C{c1[2:].upper()}")
    names = " ".join(l for l in a2.collect(0.5) if numeric(l) == 353)
    msgs = []
    if not ok:
        msgs.append("pseudos sensibles à la casse (Alice != alice)")
    if b2.nick not in names:
        msgs.append("noms de channel sensibles à la casse")
    if msgs:
        raise Warn(" ; ".join(msgs))


@test(S, "Ligne vide, espaces, commande inconnue -> 421, pas de crash")
def _(x):
    a = x.client()
    a.send(""); a.send("    ")
    empty = a.collect(0.5)
    a.send("FOOBAR arg"); need_has(a, "421", "FOOBAR")
    expect(a.alive(), "client mort")
    if 421 in nums(empty):
        raise Warn("une ligne vide genere un 421 (devrait etre ignoree)")


@test(S, "Espaces multiples entre paramètres")
def _(x):
    a, b = x.client(), x.client(); ch = x.chan()
    a.send(f"PRIVMSG   {b.nick}   :multi   spaces"); need_has(b, "multi   spaces")
    a.send(f"JOIN     {ch}"); need(a, 366)


@test(S, "Lignes très longues (600, 5000, 100000) : serveur survit")
def _(x):
    for size in (600, 5000, 100000):
        c = x.client(); c.send("PRIVMSG " + c.nick + " :" + "A" * size); c.collect(0.4)
        c.send("X" * size); c.collect(0.4)
    expect(x.server_alive(), "serveur mort après ligne longue")


@test(S, "Octets nuls / binaire aléatoire : serveur survit")
def _(x):
    c = x.client()
    c.raw(b"PRIVMSG " + c.nick.encode() + b" :a\x00b\r\n")
    c.raw(bytes(random.getrandbits(8) for _ in range(2000)) + b"\r\n")
    c.collect(0.5)
    expect(x.server_alive(), "serveur mort après binaire")


@test(S, "Client lent (ne lit jamais) ne bloque pas les autres")
def _(x):
    ch = x.chan()
    slow = x.client(); x.join(slow, ch)
    snd, obs = x.client(), x.client()
    x.join(snd, ch); snd.drain(0.2)
    snd.sock.settimeout(5)
    stop = {"n": 0}

    def flood():
        line = f"PRIVMSG {ch} :{'F' * 2000}\r\n".encode()
        for _ in range(5000):
            try:
                snd.sock.sendall(line); stop["n"] += 1
            except OSError:
                break

    t = threading.Thread(target=flood, daemon=True); t.start()
    time.sleep(1.0)
    expect(obs.alive(), "un autre client ne reçoit plus de PONG pendant le flood (serveur bloqué ?)")
    t.join(timeout=20)
    expect(x.server_alive(), "serveur mort après flood")


@test(S, "Stress : N clients simultanés (option --stress)")
def _(x):
    n = x.a.stress
    ch = "#stress" + rnd(3)
    results, lock = [], threading.Lock()

    def worker(i):
        ok = False
        try:
            c = Client(x.a.host, x.a.port)
            nick = f"s{i}{rnd(4)}"
            c.send(f"PASS {x.a.password}"); c.send(f"NICK {nick}"); c.send(f"USER {nick} 0 * :S")
            good, _ = c.wait_num(1, timeout=10)
            if good:
                c.send(f"JOIN {ch}")
                good, _ = c.wait_num(366, timeout=10)
                for k in range(5):
                    c.send(f"PRIVMSG {ch} :msg {k}")
                c.collect(0.5, maxtime=4)
                ok = good and not c.closed
            c.close()
        except Exception:
            ok = False
        with lock:
            results.append(ok)

    ts = [threading.Thread(target=worker, args=(i,)) for i in range(n)]
    for t in ts: t.start()
    for t in ts: t.join()
    good = sum(results)
    expect(good == n, f"{good}/{n} clients OK")
    time.sleep(0.5)
    expect(x.server_alive(), "serveur mort après le stress")


@test(S, "Le serveur répond toujours à la fin")
def _(x):
    expect(x.server_alive(), "serveur mort")


# --------------------------------------------------------------------------- #
#  Lanceur
# --------------------------------------------------------------------------- #

COL = {"PASS": "\033[32m", "FAIL": "\033[31m", "WARN": "\033[33m", "SKIP": "\033[36m"}
END = "\033[0m"


def main():
    p = argparse.ArgumentParser(description="Tests automatisés d'un serveur IRC")
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--port", type=int, default=6667)
    p.add_argument("--password", default="password", help="mot de passe du serveur")
    p.add_argument("--oper-user", default="admin", help="username utilisé pour OPER")
    p.add_argument("--oper-pass", default=None, help="mot de passe OPER (active les tests opérateur)")
    p.add_argument("--stress", type=int, default=50, help="nb de clients du test de stress")
    p.add_argument("--keepalive", type=int, default=0, help="durée (s) du test d'inactivité")
    p.add_argument("--only", default=None, help="ne lance que les tests dont section/nom contient ce texte")
    p.add_argument("--list", action="store_true", help="liste les tests")
    p.add_argument("--no-color", action="store_true")
    args = p.parse_args()

    if args.list:
        for s, n, _ in TESTS:
            print(f"{s:22} {n}")
        return 0

    if args.no_color:
        COL.clear()

    try:
        socket.create_connection((args.host, args.port), timeout=2).close()
    except OSError as e:
        print(f"Impossible de se connecter à {args.host}:{args.port} ({e})")
        return 2

    counts = {"PASS": 0, "FAIL": 0, "WARN": 0, "SKIP": 0}
    last_section = None
    selected = [t for t in TESTS
                if not args.only or args.only.lower() in (t[0] + " " + t[1]).lower()]

    for section, name, fn in selected:
        if section != last_section:
            print(f"\n=== {section} ===")
            last_section = section
        x = Ctx(args)
        status, detail = "PASS", ""
        t0 = time.time()
        try:
            fn(x)
        except Skip as e:
            status, detail = "SKIP", str(e)
        except Warn as e:
            status, detail = "WARN", str(e)
        except AssertionError as e:
            status, detail = "FAIL", str(e)
        except Exception as e:
            status = "FAIL"
            detail = f"exception {type(e).__name__}: {e}"
            if "-v" in sys.argv:
                detail += "\n" + traceback.format_exc()
        finally:
            x.close_all()
        counts[status] += 1
        col = COL.get(status, "")
        print(f"[{col}{status}{END if col else ''}] {name}  ({time.time() - t0:.1f}s)")
        if detail:
            print(f"        -> {detail}")
        time.sleep(0.3)
        try:
            socket.create_connection((args.host, args.port), timeout=1).close()
        except OSError:
            print(f"\n!!! LE SERVEUR NE REPOND PLUS juste apres le test : '{name}'. Arret.")
            print("    -> regarde son code de sortie / stderr (SIGPIPE ? segfault ?), lance-le sous valgrind/gdb.")
            counts["FAIL"] += 1
            break

    print("\n" + "=" * 50)
    print("  ".join(f"{k}: {v}" for k, v in counts.items()))
    if FORMAT_ISSUES:
        print("\nANOMALIES DE FORMAT DETECTEES DANS LES REPONSES DU SERVEUR :")
        for kind, exs in FORMAT_ISSUES.items():
            print(f"  - {kind}")
            for e in exs:
                print(f"      {e}")
    print("\nÀ tester à la main : Ctrl+C sur le serveur puis relance immédiate (SO_REUSEADDR),")
    print("valgrind --leak-check=full, `lsof -p PID` (fd qui redescendent), vrais clients (irssi/WeeChat/HexChat).")
    return 1 if counts["FAIL"] else 0


if __name__ == "__main__":
    sys.exit(main())
