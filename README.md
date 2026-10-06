*This project has been created as part of the 42 curriculum by mbah, zcherif.*

# ft_irc

## Description

`ft_irc` is an IRC server written in C++98 as part of the 42 curriculum. The goal is to build a TCP server that allows multiple IRC clients to connect, authenticate, and communicate using the IRC protocol.

The server handles concurrent connections with `poll()` and supports user registration, private messages, and channels. It implements commands including `PASS`, `NICK`, `USER`, `JOIN`, `PART`, `PRIVMSG`, `NOTICE`, `TOPIC`, `KICK`, `INVITE`, `MODE`, `WHO`, `NAMES`, `LIST`, `PING`, `PONG`, and `QUIT`. Supported channel modes are `i` (invite-only), `t` (topic restricted to operators), `k` (key), `o` (operator), and `l` (member limit).

## Instructions

### Requirements

- A Unix/Linux or macOS environment with POSIX sockets
- `make`
- A C++98-compatible compiler (such as `c++` or `g++`)
- Python 3 to run the provided test suite

No external libraries are required to build the server.

### Compilation

From the repository root, build the project with:

```sh
make
```

The `ircserv` executable is created in the repository root. To remove the object files and executable:

```sh
make fclean
```

To rebuild the project:

```sh
make re
```

### Running the server

The server takes two arguments: the listening port and the connection password.

```sh
./ircserv <port> <password>
```

Example:

```sh
./ircserv 6667 mypassword
```

The port must be between 1 and 65535, and the password must not be empty. From another terminal, connect to `127.0.0.1` on the selected port using an IRC client such as `weechat`, then provide the password and your identity.
You will have to use thoses two commands : 

    /server add irc_test 127.0.0.1/<PORT_LISTENNIG> -password=<PASSWORD> -nicks=alice -username=alice -realname="Alice test"
    /connect irc_test

Alternatively, you can use a command-line client that preserves IRC line endings:

```sh
nc -C 127.0.0.1 6667
```

Once connected, send the following commands, replacing the password and nickname as needed:

```text
PASS mypassword
NICK alice
USER alice 0 * :Alice
JOIN #42
PRIVMSG #42 :Hello!
```

### Running the tests

First start the server with the port and password you intend to use for testing. In another terminal, run:

```sh
python3 test_irc.py --port 6667 --password mypassword
```

The script can also list available tests (`--list`), filter the tests to run (`--only MODE`), and configure operator credentials (`--oper-user`, `--oper-pass`). Run `python3 test_irc.py --help` to see all available options.

## Resources

### References

- [RFC 1459 — Internet Relay Chat Protocol](https://www.rfc-editor.org/rfc/rfc1459): the original IRC protocol specification.
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812): IRC client commands and message exchanges.
- [RFC 2811 — Internet Relay Chat: Channel Management](https://www.rfc-editor.org/rfc/rfc2811): channel management and modes.
- [The Open Group — `poll()`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/poll.html): documentation for the I/O multiplexing API used to monitor sockets.
- Local manual pages: `man 2 socket`, `man 2 bind`, `man 2 listen`, and `man 2 poll`.

### AI usage

AI was used to structure and write this README, including the project overview, build and run instructions, and protocol references. It also helped review the configuration files, `Makefile`, and commands implemented in the source code to document the project. For this documentation task, AI did not generate or modify any part of the server's source code.
