# ft_irc

Base de serveur IRC en C++98 pour le projet 42.

Cette version pose uniquement le socle reseau. Elle ouvre la socket, accepte
plusieurs clients, fonctionne en mode non-bloquant et ferme proprement toutes
les connexions. Aucune commande IRC n'est encore implantee ici.

## Objectif de cette etape

L'objectif est d'avoir une base simple, lisible et solide avant d'ajouter le
parseur de commandes, la gestion des channels et les permissions.

Le serveur doit:

- ouvrir une socket TCP sur un port donné;
- ecouter les connexions entrantes;
- accepter plusieurs clients simultanement;
- utiliser `poll()` pour eviter de bloquer;
- fermer proprement les sockets quand un client part ou quand le serveur
	s'arrete.

### `Server`

`Server` porte toute la logique reseau:

- creation de la socket d'ecoute;
- configuration de `SO_REUSEADDR` (pouvoir réutiliser un port rapidement, même s’il est encore “bloqué” par le système.);
- passage en non-bloquant;
- boucle principale avec `poll()`;
- acceptation des nouveaux clients;
- fermeture propre des clients et de la socket d'ecoute.

### `User`

`User` represente un client connecte. Dans cette phase, il contient:

- le descripteur de socket;
- l'adresse d'origine;
- un tampon d'entree (message) prepare pour la suite.

## Pourquoi ca marche

Le point critique d'un serveur IRC est de ne jamais attendre indefiniment sur
un seul client. Ici, la socket d'ecoute et les sockets clients sont toutes en
mode non-bloquant. Ensuite `poll()` indique quels descripteurs sont pret a lire
ou s'il faut nettoyer une connexion en erreur.

Le resultat est un serveur qui peut garder plusieurs connexions ouvertes sans
se figer.

## Compilation

Depuis la racine du projet:

```bash
make
```

Le binaire genere est `ircserv`.

## Execution

```bash
./ircserv <port> <password>
```

Exemple:

```bash
./ircserv 6667 password
```

Le port doit etre compris entre 1 et 65535 (2^16) et le mot de passe ne peut pas etre
vide.

## Comment tester

### 1. Demarrer le serveur

```bash
./ircserv 6667 secret
```

### 2. Ouvrir un client brut

Avec `nc` ou `telnet`:

```bash
nc 127.0.0.1 6667
```

### 3. Verifier le comportement attendu

- plusieurs `nc` peuvent se connecter en meme temps;
- fermer une fenetre client ne doit pas faire planter le serveur;
- `Ctrl+C` doit arreter proprement le serveur;
- le serveur ne doit pas bloquer sur un seul client.

### 4. Verifier la non-bloquance

Laisser un client ouvert sans rien envoyer pendant que d'autres se connectent.
Le serveur doit continuer a accepter et gerer les autres clients.

## Ce qui sera ajoute ensuite

Quand cette base est stable, la suite logique est:

1. parser les lignes IRC;
2. gerer `PASS`, `NICK` et `USER`;
3. ajouter les channels;
4. implementer `JOIN`, `PART` et `PRIVMSG`;
5. ajouter les modes et les commandes operateur.

Cette separation permet de garder le projet maintenable et facile a faire
evoluer sans casser le reseau de base.
