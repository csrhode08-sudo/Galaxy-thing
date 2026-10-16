#!/usr/bin/env python3
"""Copy big-endian field annotations from Dusklight's JSystem headers onto Petari's.

For each struct/class in the reference header, collect fields declared as BE(T)/BE<T> together with
their `/* 0xNN */` offset comment. In the target header, for the struct with the same name, the field
at the same offset gets its type wrapped in BE(...). Only fields with offset comments are matched.

usage: apply_be.py <reference.h> <target.hpp> [--dry]
"""
import re
import sys

STRUCT_RE = re.compile(r'^\s*(?:struct|class)\s+(\w+)[^;{]*\{', re.M)
FIELD_RE = re.compile(r'^(\s*)/\*\s*(0x[0-9A-Fa-f]+)\s*\*/\s*(.+?)\s+(\**\w+)(\s*\[[^\]]*\])*\s*;', re.M)


def struct_bodies(src):
    for m in STRUCT_RE.finditer(src):
        i = m.end() - 1
        depth = 0
        j = i
        while j < len(src):
            if src[j] == '{':
                depth += 1
            elif src[j] == '}':
                depth -= 1
                if depth == 0:
                    break
            j += 1
        yield m.group(1), m.end(), j


def top_level(src, a, b):
    """Yields (start, end) spans of [a, b) that are not inside nested braces (nested structs, function bodies)."""
    depth = 0
    start = a
    for i in range(a, b):
        c = src[i]
        if c == '{':
            if depth == 0 and i > start:
                yield start, i
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                start = i + 1
    if depth == 0 and start < b:
        yield start, b


def field_matches(src, a, b):
    for s0, s1 in top_level(src, a, b):
        yield from FIELD_RE.finditer(src, s0, s1)


def be_fields(src):
    out = {}
    for name, a, b in struct_bodies(src):
        fields = {}
        for f in field_matches(src, a, b):
            typ = f.group(3).strip()
            mm = re.match(r'(?:const\s+)?BE[(<]\s*([\w\s:]+?)\s*[)>]\s*(\**)$', typ)
            if mm:
                fields[int(f.group(2), 16)] = (mm.group(1), mm.group(2))
            elif typ.startswith('OFFSET_PTR'):
                fields[int(f.group(2), 16)] = typ
        if fields:
            out.setdefault(name, {}).update(fields)
    return out


def main():
    ref, tgt = sys.argv[1], sys.argv[2]
    dry = '--dry' in sys.argv
    refs = be_fields(open(ref).read())
    src = open(tgt).read()
    edits = {}
    for name, a, b in struct_bodies(src):
        if name not in refs:
            continue
        for f in field_matches(src, a, b):
            off = int(f.group(2), 16)
            typ = f.group(3).strip()
            if off not in refs[name] or 'BE(' in typ or 'BE<' in typ:
                continue
            reftyp = refs[name][off]
            if isinstance(reftyp, str):
                # A file offset the console turns into a pointer in place: keep it a 32-bit BE offset on PC.
                if typ.rstrip().endswith('*'):
                    s0 = f.start(3)
                    edits[(s0, s0 + len(f.group(3)))] = 'OFFSET_PTR_V0'
                else:
                    print(f'  {name}+0x{off:X}: {typ} vs {reftyp}; skipped')
                continue
            base, stars = reftyp
            const = ''
            t = typ
            if t.startswith('const '):
                const, t = 'const ', t[6:]
            tstars = len(t) - len(t.rstrip('*'))
            t = t.rstrip('*').strip()
            if tstars != len(stars):
                print(f'  {name}+0x{off:X}: pointer depth differs ({typ} vs BE({base}){stars}); skipped')
                continue
            s0 = f.start(3)
            edits[(s0, s0 + len(f.group(3)))] = f'{const}BE({t}){"*" * tstars}'
    for (s0, s1), text in sorted(edits.items(), reverse=True):
        src = src[:s0] + text + src[s1:]
    print(f'{tgt}: {len(edits)} fields converted')
    if not dry:
        open(tgt, 'w').write(src)


if __name__ == '__main__':
    main()
