#!/usr/bin/env python3
"""Disassemble a function from the original main.dol: dis.py <symbol-substring> [dol] [symbols.txt]"""
import sys, struct, re, subprocess
sym = sys.argv[1]
dol = open(sys.argv[2] if len(sys.argv) > 2 else '/home/claude/Petari/orig/RMGK01/sys/main.dol', 'rb').read()
syms = open(sys.argv[3] if len(sys.argv) > 3 else '/home/claude/Petari/config/RMGK01/symbols.txt').read()
offs = struct.unpack('>18I', dol[0:72]); addrs = struct.unpack('>18I', dol[72:144]); sizes = struct.unpack('>18I', dol[144:216])
def read(addr, n):
    for o, a, s in zip(offs, addrs, sizes):
        if a <= addr < a + s: return dol[o + addr - a:o + addr - a + n]
symtab = {}
for m in re.finditer(r'^(\S+) = \.\w+:0x([0-9A-F]+); // type:(\w+)(?: size:0x([0-9A-F]+))?', syms, re.M):
    symtab[int(m.group(2), 16)] = m.group(1)
for m in re.finditer(r'^(\S+) = \.text:0x([0-9A-F]+); // type:function size:0x([0-9A-F]+)', syms, re.M):
    if sym in m.group(1):
        a, n = int(m.group(2), 16), int(m.group(3), 16)
        print(f'== {m.group(1)} @ {a:08X}')
        code = read(a, n)
        hexs = ' '.join(f'0x{b:02x}' for b in code)
        out = subprocess.run(['llvm-mc', '--disassemble', '-triple=powerpc', '-mcpu=750'], input=hexs, capture_output=True, text=True).stdout
        lines = [l.strip() for l in out.splitlines() if l.strip() and not l.strip().startswith('.')]
        for k, l in enumerate(lines):
            pc = a + 4 * k
            mm = re.match(r'(bl|b|bt|bf|beq|bne|blt|bgt|ble|bge)\s+(?:\d+,\s*)?\.([+-]\d+)', l)
            extra = ''
            if mm:
                tgt = pc + int(mm.group(2))
                extra = f'  -> {tgt:08X} {symtab.get(tgt, "")}'
            print(f'  {pc:08X}: {l}{extra}')
