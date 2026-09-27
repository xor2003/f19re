#!/bin/bash
# Z3 sweep: rebuild each module's test exe, then z3check all its routines.
# Resumable: skips modules that already produced a compare.json (unless FORCE=1).
cd "$(dirname "$0")/.." || exit 1
while IFS='|' read -r src names; do
    base=$(basename "$src" .c | tr 'a-z' 'A-Z' | cut -c1-8)
    out="build/z3cmp/${base}.compare.json"
    if [ -s "$out" ] && [ "$FORCE" != "1" ]; then
        echo "########## $src ########## (skip: $out exists)"
        continue
    fi
    first=${names%%,*}
    echo "########## $src ##########"
    echo "--- portcheck (rebuild exe) $first"
    python3 tools/portcheck.py "$src" "$first" 2>&1 | tail -3
    echo "--- z3check"
    IFS=',' read -ra ns <<< "$names"
    python3 tools/z3check.py "$src" "${ns[@]}" 2>&1 | grep -v '^== discover\|^== ssa\|^== mapping'
done < build/z3mods.txt
echo "########## SWEEP DONE ##########"
