#!/usr/bin/env bash
#
# Enforce the release-retention policy after a tag is published.
#
#   RC     (vX.Y.Z-rc / vX.Y.Z-rc.N)  pre-release; only the newest RC is kept.
#                                      Publishing a stable release removes ALL RCs.
#   patch  (vX.Y.Z, Z>0)              keeps only the newest release of that X.Y line.
#   minor  (vX.Y.0)                   additive; removes nothing on its own.
#   major  (vX.0.0)                   for every PREVIOUS major, keep only that major's
#                                      highest minor line; drop the rest.
#
# The keep-set is recomputed from scratch each run (idempotent), so the same rules
# hold no matter the order tags were pushed. The current tag is never deleted.
#
# Usage:  prune-releases.sh <current-tag>
# Env:    GH_TOKEN (required, gh auth)   DRY_RUN=1 (print, don't delete)
# Needs:  gh, python3   —   the current tag's release MUST already exist on GitHub.

set -euo pipefail

CURRENT="${1:-${GITHUB_REF_NAME:-}}"
[ -n "$CURRENT" ] || { echo "::error::no tag given"; exit 1; }
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

TAGS="$(gh release list --limit 500 --json tagName --jq '.[].tagName')"
[ -n "$TAGS" ] || { echo "No releases found — nothing to prune."; exit 0; }

# Python decides which tags to remove; one tag per line on stdout.
TO_DELETE="$(printf '%s\n' "$TAGS" | python3 "$HERE/prune_releases.py" "$CURRENT")"

if [ -z "$TO_DELETE" ]; then echo "Nothing to prune (policy already satisfied)."; exit 0; fi

while IFS= read -r tag; do
  [ -n "$tag" ] || continue
  if [ "${DRY_RUN:-0}" = "1" ]; then
    echo "DRY-RUN  would remove  $tag"
  else
    echo "Removing release + tag  $tag"
    gh release delete "$tag" --yes --cleanup-tag
  fi
done <<< "$TO_DELETE"
