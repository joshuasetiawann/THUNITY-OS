#!/usr/bin/env bash
# THUOS — honesty doctrine guard.
#
# Fails if any forbidden capability claim appears as a POSITIVE statement in the
# tracked text/code. THUOS must never claim it runs Thunity AI, Docker, Python,
# Node, Ollama, real networking, or a verified AI runtime — because it does not.
# Negated or clearly-qualified mentions (no/not/cannot/without/design-only/
# planned/long-term/not yet/...) ARE allowed, so the honest docs can discuss
# these limits openly.
set -u
cd "$(dirname "$0")/.."

# Exact phrases that would be dishonest if stated positively.
phrases=(
  "runs thunity ai"
  "run thunity ai natively"
  "runs ai natively"
  "run ai natively"
  "docker supported"
  "supports docker"
  "python supported"
  "node supported"
  "ollama supported"
  "real networking"
  "networking works"
  "ai runtime verified"
)

# Words/markers that make a mention clearly non-claiming (allowed).
neg='no |not |cannot|can.t|without|never|isn.t|does not|doesn.t|do not|don.t|design-only|design only|not yet|no .* yet|long-term|long term|planned|future|roadmap|would |cannot currently|will need|requires '

files=$(git ls-files '*.md' '*.c' '*.h' '*.html' '*.txt' 'README*' 'CHANGELOG*' 2>/dev/null)
[ -z "$files" ] && { echo "overclaim scan: no tracked files"; exit 0; }

bad=0
for p in "${phrases[@]}"; do
  while IFS= read -r hit; do
    [ -z "$hit" ] && continue
    low=$(printf '%s' "$hit" | tr '[:upper:]' '[:lower:]')
    # allow if the line carries a negation / qualification cue
    if printf '%s' "$low" | grep -Eq "$neg"; then continue; fi
    echo "OVERCLAIM: $hit"
    bad=1
  done < <(grep -rin -- "$p" $files 2>/dev/null)
done

if [ "$bad" -ne 0 ]; then
  echo "FAIL: positive overclaims found — fix wording (see honesty doctrine)."
  exit 1
fi
echo "overclaim scan: CLEAN (no positive claims of Thunity/Docker/Python/Node/Ollama/networking/AI-runtime)"
exit 0
