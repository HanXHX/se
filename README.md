se
==

Fast connection to SSH

[![CI](https://github.com/HanXHX/se/actions/workflows/ci.yml/badge.svg)](https://github.com/HanXHX/se/actions/workflows/ci.yml)

`se` reads your `~/.ssh/config`, prints your hosts in colored columns and
runs `ssh` on the one you pick, by number, by favorite letter or by fuzzy
pattern (`w3` matches `web-prod-03.example.com`).

Usage
-----

```
se                  # display the list, then read the selection on stdin
se 3                # connect to host number 3
se a                # connect to favorite "A"
se w3               # fuzzy match
se -c other-config  # use another ssh config file
```

See `se(1)` for the options and `se(5)` for the config format. In short,
regular `Host` entries are listed, `Include` directives are followed, and a
few comments extend the list:

```
#MYHOST admin@linux-02      # extra host, selectable by number
#PREFHOST admin@linux-02    # favorite, selectable by a letter (26 max)
#EXCLUDE relay              # hide a Host entry (put it at the end)
```

Build
-----

```
make            # builds ./se and the man pages (needs gcc, make, perl)
make test       # runs the test suite
make deb        # builds the Debian package with git-buildpackage
```

Coding style
------------

Use `indent -linux src/se.c`
