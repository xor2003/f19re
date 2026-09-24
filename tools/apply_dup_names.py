#!/usr/bin/env python3
"""Rename routines in a mzretools .map file to the names of matched
f15se2 routines, based on the 'potential duplicate' comments emitted
by mzdup.

Usage: apply_dup_names.py file.map

- For each routine that carries a comment of the form
    # Routine NAME SEG:OFF-SEG:OFF[...] is a potential duplicate of routine F15NAME ...
  the routine is renamed to F15NAME.
- When several comments reference different f15 names (the routine matched
  equally-named functions from several executables), the first name wins.
- Name collisions inside the map are resolved by appending _2, _3, ...
- The 'duplicate' marker and the comments are preserved.
"""
import re
import sys

DUP_RE = re.compile(
    r"^# Routine (\S+) \S+ is a potential duplicate of routine (\S+) ")
ROUTINE_RE = re.compile(r"^([A-Za-z_.$][\w.$]*):")


def main(path: str) -> None:
    with open(path, encoding="utf-8", errors="replace") as f:
        lines = f.read().splitlines()

    # first pass: routine_name -> suggested new name (first comment wins)
    pending = {}          # routine name -> new name
    order = []            # keep deterministic order
    last_name = None
    for line in lines:
        m = DUP_RE.match(line)
        if m:
            last_name = m.group(1)
            if last_name not in pending:
                pending[last_name] = m.group(2)
                order.append(last_name)
            continue
        rm = ROUTINE_RE.match(line)
        if rm:
            last_name = None

    # second pass: apply renames, dedupe
    used = set()
    renamed = 0
    out = []
    for line in lines:
        rm = ROUTINE_RE.match(line)
        if rm and rm.group(1) in pending:
            old = rm.group(1)
            new = pending[old]
            base = new
            i = 2
            while new in used:
                new = f"{base}_{i}"
                i += 1
            used.add(new)
            line = new + line[len(old):]
            renamed += 1
        else:
            m2 = ROUTINE_RE.match(line)
            if m2:
                used.add(m2.group(1))
        out.append(line)

    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")
    print(f"{path}: renamed {renamed} routines")


if __name__ == "__main__":
    for p in sys.argv[1:]:
        main(p)
