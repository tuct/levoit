def crc16(d):
    c=0xFFFF
    for b in d:
        c^=b<<8
        for _ in range(8):
            c=((c<<1)^0x1021)&0xFFFF if c&0x8000 else (c<<1)&0xFFFF
    return c
pkts=[
 ("QUERY 0x0004","08 00","94 A6"),
 ("STATUS 0x0007","00 08 01 02 00 00 02 01 00 03 73 06 30 2E 33 2E 33 00 04 02 00 00 05 02 00 00 07 02 00 00 00","E1 1A"),
 ("SET 0x0003","08 02 01 00 00","C9 79"),
 ("HS1 0x0001","03 00","48 5C"),
]
for n,d,c in pkts:
    data=bytes.fromhex(d.replace(" ",""))
    want=int(c.replace(" ",""),16)
    got=crc16(data)
    print(f"{n:15s} len={len(data):2d} calc={got:04X} wire={want:04X} {'OK' if got==want else 'MISMATCH'}")
