#!/usr/bin/env python3
"""Apply conf/routine_names.txt renames to a mzretools map file."""
import re
import sys

def main(map_path, names_path):
    renames = {}
    with open(names_path, errors='replace') as f:
        for line in f:
            line = line.split('#')[0].strip()
            if not line:
                continue
            old, new = line.split()[:2]
            renames[old] = new
    out = []
    with open(map_path, errors='replace') as f:
        for line in f:
            m = re.match(r'([A-Za-z_.$][\w.$]*):', line)
            if m and m.group(1) in renames:
                line = renames[m.group(1)] + ':' + line[m.end():]
            out.append(line)
    with open(map_path, 'w') as f:
        f.write(''.join(out))

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
