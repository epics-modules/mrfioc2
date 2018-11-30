#!/usr/bin/env python3
"""
Read header of Xilinx .bit files.

./bitinfo.py some.bit
"""

import sys
import struct

for fname in sys.argv[1:]:
    info = {}

    with open(fname, 'rb') as F:
        if F.read(13)!=b'\0\x09\x0f\xf0\x0f\xf0\x0f\xf0\x0f\xf0\0\0\1':
            print("Not a bit file")
            sys.exit(1)

        while True:
            head = F.read(1)
            if len(head)==0:
                break

            if head[0] in (0x61, 0x62, 0x63, 0x64):
                size, = struct.unpack('!H', F.read(2))
                body = F.read(size)
                assert len(body)==size

                info[head[0]] = body.decode('utf-8').rstrip('\0')

            elif head[0] in (0x65,):
                size, = struct.unpack('!I', F.read(4))
                F.seek(size, 1) # skip
                info[head[0]] = size

            else:
                raise RuntimeError("Unknown %s at %s"%(head[0], F.tell()))

    print(fname)
    print("  Description:", info.pop(0x61, '?'))
    print("  Part:", info.pop(0x62, '?'))
    print("  Build date:", info.pop(0x63, '?'), info.pop(0x64, '?'))
    print("  Bit stream length:", info.pop(0x65, 0))

    if info:
        print("  Extra:", info)
