#!/usr/bin/env python3
"""Minimal Yaz0 + RARC extractor. usage: rarc.py <file.arc> <outdir>  (or: rarc.py <file.arc> to list)"""
import os, struct, sys

def yaz0(d):
    if d[:4] != b'Yaz0':
        return d
    size = struct.unpack('>I', d[4:8])[0]
    out = bytearray(); s = 16
    while len(out) < size:
        code = d[s]; s += 1
        for bit in range(8):
            if len(out) >= size: break
            if code & (0x80 >> bit):
                out.append(d[s]); s += 1
            else:
                b1, b2 = d[s], d[s+1]; s += 2
                dist = ((b1 & 0xF) << 8 | b2) + 1
                n = b1 >> 4
                if n == 0: n = d[s] + 0x12; s += 1
                else: n += 2
                for _ in range(n): out.append(out[-dist])
    return bytes(out)

def rarc(d):
    assert d[:4] == b'RARC', d[:4]
    hdr = struct.unpack('>8I', d[:32])
    info = 0x20
    ndirs, diroff, nfiles, fileoff, strsz, stroff = struct.unpack('>6I', d[info:info+24])
    dataoff = hdr[3] + hdr[2]
    strs = info + stroff
    def name(o): return d[strs+o:d.index(b'\0', strs+o)].decode('ascii', 'replace')
    files = []
    for i in range(nfiles):
        e = info + fileoff + i * 20
        fid, h, fl, doff, dsz = struct.unpack('>HHIII', d[e:e+16])
        files.append((fl >> 24, name(fl & 0xFFFFFF), doff, dsz))
    out = {}
    def walk(di, path):
        e = info + diroff + di * 16
        _, nmo, _, cnt, first = struct.unpack('>IIHHI', d[e:e+16])
        for k in range(first, first + cnt):
            fl, nm, doff, dsz = files[k]
            if nm in ('.', '..'): continue
            if fl & 2: walk(doff, path + nm + '/')
            else: out[path + nm] = d[dataoff + doff: dataoff + doff + dsz]
    walk(0, '')
    return out

if __name__ == '__main__':
    files = rarc(yaz0(open(sys.argv[1], 'rb').read()))
    for k, v in files.items():
        if len(sys.argv) > 2:
            p = os.path.join(sys.argv[2], k); os.makedirs(os.path.dirname(p), exist_ok=True); open(p, 'wb').write(v)
        else:
            print(f'{len(v):8} {k}')
