#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  ./bench/bench.sh --width W --height H --steps S --seed N --history-cap C

Exemple:
  ./bench/bench.sh --width 512 --height 512 --steps 2000 --seed 42 --history-cap 512
EOF
}

WIDTH=""
HEIGHT=""
STEPS=""
SEED=""
HCAP=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --width) WIDTH="${2:-}"; shift 2;;
    --height) HEIGHT="${2:-}"; shift 2;;
    --steps) STEPS="${2:-}"; shift 2;;
    --seed) SEED="${2:-}"; shift 2;;
    --history-cap) HCAP="${2:-}"; shift 2;;
    -h|--help) usage; exit 0;;
    *) echo "Argument inconnu: $1" >&2; usage; exit 2;;
  esac
done

if [[ -z "$WIDTH" || -z "$HEIGHT" || -z "$STEPS" || -z "$SEED" || -z "$HCAP" ]]; then
  echo "Paramètres manquants." >&2
  usage
  exit 2
fi

echo "== Build =="
make -C projet-listechainee clean >/dev/null
make -C projet-listechainee bench >/dev/null
make -C projet-ringbuffer clean >/dev/null
make -C projet-ringbuffer bench >/dev/null

CMD_ARGS=(--width "$WIDTH" --height "$HEIGHT" --steps "$STEPS" --seed "$SEED" --history-cap "$HCAP")

echo "== Run =="
OUT_LIST="$(./projet-listechainee/bin/life_bench "${CMD_ARGS[@]}")"
OUT_RING="$(./projet-ringbuffer/bin/life_bench "${CMD_ARGS[@]}")"

echo "$OUT_LIST"
echo "$OUT_RING"

get_field() {
  local line="$1"
  local key="$2"
  echo "$line" | awk -v k="$key" '
    {
      for (i=1; i<=NF; i++) {
        split($i, a, "=");
        if (a[1] == k) { print a[2]; exit 0; }
      }
    }'
}

SPS_LIST="$(get_field "$OUT_LIST" "steps_per_s")"
SPS_RING="$(get_field "$OUT_RING" "steps_per_s")"
T_LIST="$(get_field "$OUT_LIST" "total_s")"
T_RING="$(get_field "$OUT_RING" "total_s")"
NS_LIST="$(get_field "$OUT_LIST" "ns_per_step")"
NS_RING="$(get_field "$OUT_RING" "ns_per_step")"

echo "== Résumé =="
printf "listechainee: total_s=%s  steps/s=%s  ns/step=%s\n" "$T_LIST" "$SPS_LIST" "$NS_LIST"
printf "ringbuffer:   total_s=%s  steps/s=%s  ns/step=%s\n" "$T_RING" "$SPS_RING" "$NS_RING"

WINNER="$(awk -v a="$SPS_LIST" -v b="$SPS_RING" 'BEGIN{print (b>a) ? "ringbuffer" : "listechainee"}')"
echo "Gagnant (steps/s): $WINNER"
