# se — Fast connection to SSH

Small SSH front-end written in C (a single file, ~570 lines). It parses
`~/.ssh/config`, prints the host list in colored columns, and runs
`ssh -F <config> <host>` on the selected host (by number, by letter, by
pattern, or from the command line).

Personal project started in 2013, dormant from January 2019 (v1.0-beta4)
until a cleanup pass in September 2026. License: GPLv2. Author: Emilien Mantel.

## Layout

```
src/se.c            All the code (host linked list, config parsing incl. Include,
                    fuzzy matching, fork/exec ssh)
src/Makefile        Builds ../se with $(CC) -O2 -Wall -Wextra -Werror; honors CFLAGS/LDFLAGS
tests/run.sh        Functional tests (fake config + /bin/echo instead of ssh)
doc/pod/*.pod       Man page sources (en/fr, sections 1 and 5)
doc/create-manpages.pl   Generates doc/man/** with pod2man (doc/man/ is git-ignored)
debian/             Debian packaging (debhelper-compat 13, gbp)
.github/workflows/ci.yml   Build, tests (native + ASan/UBSan), Debian package
VERSION             Version injected into the binary (-DVERSION) and the man pages
Makefile            Top level: all = clean + documentation + bin ; test ; deb = gbp buildpackage
builds/             Output of `make deb` (git-ignored, removed by `make clean`)
```

## Build and test

```bash
make            # generates the man pages then builds ./se (runs clean first)
make test       # runs tests/run.sh against ./se
make deb        # Debian package through gbp, output in builds/
./se -v         # sanity check
```

Dependencies: gcc, make, perl + pod2man. For the package: debhelper,
git-buildpackage, devscripts. No external library.

Always run the tests, and run them under the sanitizers after touching
`src/se.c`:

```bash
make -C src clean && CFLAGS="-O1 -g -fsanitize=address,undefined" make -C src
sh tests/run.sh ./se
```

To try something by hand without actually connecting, use a fake config
file and replace ssh with echo:

```bash
./se -c /path/to/test-config -b /bin/echo w3
```

To build the Debian package outside gbp (what the CI does):

```bash
dpkg-buildpackage -us -uc -b
```

## Recognized config format (see doc/pod/se_en.5.pod)

- `Host xxx`: default host (`Host *` is ignored). Selectable by number or pattern.
- `#MYHOST user@xxx`: extra host, selectable by number only.
- `#PREFHOST user@xxx`: favorite host, selectable by a single letter (A to Z, 26 max).
- `#EXCLUDE xxx`: removes a host from the list. Put it at the end of the file.
- `Include pattern`: followed like OpenSSH does (tilde, globs, paths relative
  to `~/.ssh`), nesting capped at 16 levels.

Not supported: `Match`, several patterns on one `Host` line, wildcards other
than a lone `*`. Config lines are limited to 128 bytes, input to 255.

## Conventions

- Linux kernel style: `indent -linux src/se.c` (tabs, K&R braces).
- `-Werror` is on: any new warning breaks the build.
- Everything lives in `main()` plus a handful of global functions; no header, no modules.
- ANSI colors are hardcoded as macros at the top of the file.
- Every allocation goes through `calloc`/`strdup` with the `ALLOC_FAILURE()` macro on error.
- Errors go to stderr and `exit(EXIT_FAILURE)` on the spot; there is no error propagation.
- New behavior gets a case in `tests/run.sh`.
- Language: all code comments, documentation, README and this file are in English.
- Commits: gitmoji convention, short English message in the imperative mood
  (e.g. `🐛 Fix segfault with 26 PREFHOST entries`). History before 2026 has
  no gitmoji.

## Remaining rough edges

- The list is a singly linked list sorted by hostname (after stripping `user@`);
  hosts are rebuilt on every run, there is no cache.
- `Include` does not deduplicate: a file included twice lists its hosts twice,
  which is what OpenSSH does too.
- The fuzzy matcher only compares position by position; it is a heuristic, not
  a real edit distance.
- `-DVERSION` comes from the `VERSION` file; bump it there and in
  `debian/changelog` together.
- `indent` is not installed on every machine; keep the Linux style by hand
  if it is missing.
