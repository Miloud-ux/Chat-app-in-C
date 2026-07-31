#!/usr/bin/env bash
set -u

PORT=8080
PASS=0
FAIL=0
TMPDIR=$(mktemp -d)
SERVER_LOG="$TMPDIR/server.log"

cleanup() {
  kill "$SERVER_PID" 2>/dev/null
  pkill -f "./client" 2>/dev/null
  rm -rf "$TMPDIR"
}
trap cleanup EXIT

report() {
  local name="$1" ok="$2"
  if [ "$ok" -eq 0 ]; then
    echo "PASS: $name"
    PASS=$((PASS + 1))
  else
    echo "FAIL: $name"
    FAIL=$((FAIL + 1))
  fi
}

assert_contains() {
  local file="$1" needle="$2"
  if grep -qF "$needle" "$file"; then echo 0; else echo 1; fi
}

pkill -x server 2>/dev/null
pkill -f "./client" 2>/dev/null
sleep 0.2

make >/dev/null 2>&1 || { echo "BUILD FAILED"; exit 1; }

stdbuf -oL -eL ./server >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!
sleep 0.5

run_client() {
  local out="$1"
  shift
  (printf '%s\n' "$@"; sleep 1) | timeout 6 ./client >"$out" 2>&1
}

# --- Test 1: single client connect, message, clean :end disconnect ---
run_client "$TMPDIR/alice.log" Alice "hello everyone" ":end"
report "connect + username handshake" "$(assert_contains "$SERVER_LOG" "Alice has connected")"
report "message broadcast to server" "$(assert_contains "$SERVER_LOG" "hello everyone")"
report "client :end exits" "$(assert_contains "$TMPDIR/alice.log" "Exiting...")"
report "client detects clean EOF" "$(assert_contains "$TMPDIR/alice.log" "server disconnected")"
report "server logs disconnect" "$(assert_contains "$SERVER_LOG" "Alice disconnected")"

# --- Test 2: :online returns the user list ---
run_client "$TMPDIR/online.log" Carol ":online" ":end"
report ":online returns list" "$(assert_contains "$TMPDIR/online.log" "=== Online Users ===")"
report ":online lists the user" "$(assert_contains "$TMPDIR/online.log" "1. Carol")"

# --- Test 3: two clients, message exchange + Lamport monotonicity ---
(sleep 0.5; printf 'Alice\nmsg-a\n'; sleep 1) | timeout 6 ./client >"$TMPDIR/alice2.log" 2>&1 &
A_PID=$!
(sleep 1.0; printf 'Bob\nmsg-b\nmsg-c\n'; sleep 1) | timeout 6 ./client >"$TMPDIR/bob.log" 2>&1 &
B_PID=$!
wait "$A_PID" "$B_PID"

report "msg-a delivered to Bob" "$(assert_contains "$TMPDIR/bob.log" "msg-a")"
report "msg-b delivered to Alice" "$(assert_contains "$TMPDIR/alice2.log" "msg-b")"
report "msg-c delivered to Alice" "$(assert_contains "$TMPDIR/alice2.log" "msg-c")"

lc_values=$(grep -oP '\[LC:\K[0-9]+' "$SERVER_LOG")
monotonic=0
prev=-1
for v in $lc_values; do
  if [ "$v" -lt "$prev" ]; then monotonic=1; break; fi
  prev=$v
done
report "Lamport clocks are monotonic" "$monotonic"

# --- Test 4: raw socket client (netcat-style, no LC prefix) ---
python3 - "$SERVER_LOG" <<'EOF'
import socket, sys, time
s = socket.create_connection(('127.0.0.1', 8080))
s.sendall(b'Zed' + b'\0' * 47)
time.sleep(0.2)
s.sendall(b'raw message without prefix\n')
time.sleep(0.5)
s.sendall(b':end\n')
time.sleep(0.5)
s.close()
EOF
report "raw message without LC prefix handled" "$(assert_contains "$SERVER_LOG" "raw message without prefix")"

echo ""
echo "===================="
echo "TOTAL: $((PASS + FAIL))  PASS: $PASS  FAIL: $FAIL"
[ "$FAIL" -eq 0 ]
