#!/usr/bin/env python3
"""Make gcc 2.95's multi-line string literals legal C: a newline inside a
string literal becomes backslash-n, backslash, newline -- the same
characters in the string, and a line splice the compiler accepts (gcc 3+
refuse a raw newline in a literal). Comments, character constants and
escapes are skipped correctly. Usage: join_strings.py FILE..."""
import sys

def fix(src):
    out, i, n, changed = [], 0, len(src), 0
    while i < n:
        c = src[i]
        if src.startswith('/*', i):
            j = src.find('*/', i + 2); j = n if j < 0 else j + 2
            out.append(src[i:j]); i = j
        elif src.startswith('//', i):
            j = src.find('\n', i); j = n if j < 0 else j
            out.append(src[i:j]); i = j
        elif c == "'":
            j = i + 1
            while j < n and src[j] != "'":
                j += 2 if src[j] == '\\' else 1
            out.append(src[i:j + 1]); i = j + 1
        elif c == '"':
            j = i + 1
            buf = ['"']
            while j < n and src[j] != '"':
                if src[j] == '\\' and j + 1 < n:
                    buf.append(src[j:j + 2]); j += 2
                elif src[j] == '\n':
                    buf.append('\\n\\\n'); j += 1; changed += 1
                else:
                    buf.append(src[j]); j += 1
            buf.append('"')
            out.append(''.join(buf)); i = j + 1
        else:
            out.append(c); i += 1
    return ''.join(out), changed

for p in sys.argv[1:]:
    s = open(p, encoding='latin-1').read()
    t, k = fix(s)
    if k:
        open(p, 'w', encoding='latin-1').write(t)
        print('%s: %d' % (p, k))
