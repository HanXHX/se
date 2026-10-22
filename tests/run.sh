#!/bin/sh
# Functional tests for se. Uses a fake ssh config and /bin/echo instead of
# ssh, so nothing actually connects.
#
# Usage: tests/run.sh [path-to-se-binary]   (default: ./se)

set -u

SE=${1:-./se}
SE=$(realpath "$SE")
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAILED=0

cat > "$TMP/config" <<CONF
Host linux-01
    Hostname 127.0.0.1
Host linux-02
    Hostname 127.0.0.1
Host web-prod-03.example.com
    Hostname 10.0.0.3
Host relay
    Hostname 10.0.0.1
Host *
    ServerAliveInterval 30
Include $TMP/conf.d/*.conf
Include $TMP/does-not-exist/*
#MYHOST admin@linux-02
#PREFHOST admin@linux-02
#EXCLUDE relay
CONF
mkdir "$TMP/conf.d"
printf 'Host included-01\n    Hostname 1.2.3.4\n' > "$TMP/conf.d/a.conf"

# 26 favorites: the maximum allowed
for i in $(seq 1 26); do
	printf 'Host h%d\n#PREFHOST h%d\n' "$i" "$i"
done > "$TMP/config26"

# Include loop
printf 'Include %s/loop\n' "$TMP" > "$TMP/loop"

# Sorted list as displayed: 1) included-01  2) linux-01  3) linux-02
#                            4) admin@linux-02  5) web-prod-03.example.com
# run NAME EXPECTED_STATUS EXPECTED_PATTERN [args...]  (stdin from $STDIN)
run() {
	name=$1; want_status=$2; want=$3; shift 3
	out=$(printf '%s\n' "${STDIN:-}" | "$SE" -c "$TMP/config" -b /bin/echo "$@" 2>&1)
	status=$?
	if [ "$status" -ne "$want_status" ]; then
		echo "FAIL $name: exit $status, expected $want_status"
		echo "$out" | sed 's/^/    /'
		FAILED=1
	elif ! echo "$out" | grep -q -- "$want"; then
		echo "FAIL $name: output does not contain '$want'"
		echo "$out" | sed 's/^/    /'
		FAILED=1
	else
		echo "ok   $name"
	fi
}

STDIN=""
run "version"                 0 "^se "                      -v
run "help"                    0 "hostname-length"           -h
run "select by number"        0 "^-F .* linux-02$"          3
run "select MYHOST by number" 0 "^-F .* admin@linux-02$"    4
run "select favorite letter"  0 "^-F .* admin@linux-02$"    a
run "fuzzy w3"                0 "^-F .* web-prod-03"        w3
run "fuzzy case-insensitive"  0 "^-F .* linux-01$"          L1
run "fuzzy longer than host"  0 "^-F .* web-prod-03"        web-prod-03.example.com.tld
run "included host"           0 "^-F .* included-01$"       i1
run "excluded host"           1 "Host not found"            relay
run "unknown host"            1 "Host not found"            zz
run "bad column count"        1 "Invalid columns number"    -o 0
run "bad hostname length"     1 "Invalid hostname length"   -l 0
STDIN="2";  run "stdin number"          0 "^-F .* linux-01$"
STDIN="1";  run "split name"            0 "web-prod-03 "         -s -l 30
STDIN="";   run "empty stdin"           1 "No input"

# The list must show exactly 5 numbered hosts: "Host *" is hidden
if printf '1\n' | "$SE" -c "$TMP/config" -b /bin/echo 2>&1 | grep -q '5) web-prod-03' \
   && ! printf '1\n' | "$SE" -c "$TMP/config" -b /bin/echo 2>&1 | grep -q '6)'; then
	echo "ok   list hides Host *"
else
	echo "FAIL list hides Host *"; FAILED=1
fi

# Edge cases on other config files
if printf '1\n' | "$SE" -c "$TMP/config26" -b /bin/echo >/dev/null 2>&1; then
	echo "ok   26 favorites"
else
	echo "FAIL 26 favorites: exit $?"; FAILED=1
fi
if "$SE" -c "$TMP/loop" 1 2>&1 | grep -q "nested too deeply"; then
	echo "ok   include loop"
else
	echo "FAIL include loop"; FAILED=1
fi
if env -u HOME "$SE" 2>&1 | grep -q "HOME"; then
	echo "ok   missing HOME"
else
	echo "FAIL missing HOME"; FAILED=1
fi

exit $FAILED
