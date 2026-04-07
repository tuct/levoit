#!/usr/bin/env python3
"""
Extract files from a LittleFS partition in an ESP32 firmware dump.
Automatically locates the partition by scanning for the LittleFS magic,
or falls back to parsing the ESP32 partition table.
Requires: pip install littlefs-python

Usage:
    python extract_assets.py firmware-backup.bin
"""

import sys
import os
import struct
import littlefs

OUT_DIR = "assets_extracted"

PARTITION_TABLE_OFFSET = 0x8000
PARTITION_ENTRY_SIZE   = 32
PARTITION_MAGIC        = 0x50AA  # bytes AA 50, read as little-endian uint16
LITTLEFS_SUBTYPE       = 0x83   # ESP-IDF subtype for LittleFS


def find_partition_table(data):
    """Parse ESP32 partition table, return list of (name, type, subtype, offset, size)."""
    partitions = []
    for i in range(PARTITION_TABLE_OFFSET, PARTITION_TABLE_OFFSET + 0xC00, PARTITION_ENTRY_SIZE):
        entry = data[i:i + PARTITION_ENTRY_SIZE]
        if len(entry) < PARTITION_ENTRY_SIZE:
            break
        magic = struct.unpack_from("<H", entry, 0)[0]
        if magic != PARTITION_MAGIC:
            break
        ptype   = entry[2]
        subtype = entry[3]
        offset  = struct.unpack_from("<I", entry, 4)[0]
        size    = struct.unpack_from("<I", entry, 8)[0]
        label   = entry[12:28].rstrip(b"\x00").decode("ascii", errors="replace")
        partitions.append((label, ptype, subtype, offset, size))
    return partitions


def find_littlefs_offset(data):
    """
    Find the offset and size of a LittleFS filesystem in the dump.
    Strategy:
      1. Parse partition table and look for LittleFS subtype (0x83).
      2. Fall back to scanning for the 'littlefs' magic string.
    Returns (offset, size) or raises RuntimeError.
    """
    partitions = find_partition_table(data)
    if partitions:
        print("Partition table:")
        for name, ptype, subtype, offset, size in partitions:
            marker = " <-- LittleFS" if subtype == LITTLEFS_SUBTYPE else ""
            print(f"  {name:20s}  type=0x{ptype:02X}  sub=0x{subtype:02X}"
                  f"  offset=0x{offset:06X}  size=0x{size:06X} ({size//1024}KB){marker}")
        print()

        lfs_parts = [(p[0], p[3], p[4]) for p in partitions if p[2] == LITTLEFS_SUBTYPE]
        if lfs_parts:
            name, offset, size = lfs_parts[0]
            print(f"Found LittleFS partition '{name}' at 0x{offset:06X}, size {size//1024}KB")
            if offset + size > len(data):
                need = offset + size
                print(f"WARNING: dump is {len(data)//1024//1024:.1f} MB but partition ends at"
                      f" 0x{need:X} ({need//1024//1024} MB) — re-dump with:")
                print(f"  esptool.py --no-stub --chip esp32 -b 230400 read_flash 0"
                      f" 0x{need:X} firmware-backup.bin")
                sys.exit(1)
            return offset, size

    # Fallback: scan for 'littlefs' magic aligned to 4KB pages
    print("No LittleFS partition in table — scanning for magic...")
    for offset in range(0, len(data) - 16, 0x1000):
        if data[offset + 8:offset + 16] == b"littlefs":
            # Size: use remaining data (will be clamped later)
            size = len(data) - offset
            print(f"Found LittleFS magic at 0x{offset:06X}")
            return offset, size

    raise RuntimeError("LittleFS filesystem not found in dump")


def extract(firmware_path):
    with open(firmware_path, "rb") as f:
        data = f.read()
    print(f"Dump: {firmware_path}  ({len(data)//1024//1024:.1f} MB)\n")

    offset, size = find_littlefs_offset(data)
    assets = data[offset:offset + size]

    # Read block_size/block_count from LittleFS superblock
    magic_pos = assets.find(b"littlefs")
    if magic_pos < 0:
        print("ERROR: LittleFS magic not found inside partition")
        sys.exit(1)

    sb = assets[magic_pos:]
    block_size  = struct.unpack_from("<I", sb, 12)[0]
    block_count = struct.unpack_from("<I", sb, 16)[0]

    # Sanity check / fallback — superblock values are TLV-encoded, not a raw struct
    if block_size not in (256, 512, 1024, 2048, 4096, 8192) or block_count == 0 or block_count > size // 256:
        block_size  = 4096
        block_count = size // block_size
        print(f"Using block_size={block_size}, block_count={block_count} (derived from partition size)")

    actual_size = block_size * block_count
    if actual_size > len(assets):
        print(f"ERROR: filesystem needs {actual_size//1024}KB but only {len(assets)//1024}KB available in dump")
        sys.exit(1)

    print(f"LittleFS: block_size={block_size}  block_count={block_count}  total={actual_size//1024}KB\n")

    fs = littlefs.LittleFS(
        block_size=block_size,
        block_count=block_count,
        read_size=16,
        prog_size=16,
    )
    fs.context.buffer = bytearray(assets[:block_size * block_count])
    fs.mount()

    os.makedirs(OUT_DIR, exist_ok=True)
    extracted = 0

    def walk(path="/"):
        nonlocal extracted
        for entry in fs.scandir(path):
            full = path.rstrip("/") + "/" + entry.name
            if entry.type == 2:  # directory
                local_dir = os.path.join(OUT_DIR, full.lstrip("/"))
                os.makedirs(local_dir, exist_ok=True)
                walk(full)
            else:
                local_path = os.path.join(OUT_DIR, full.lstrip("/").replace("/", os.sep))
                os.makedirs(os.path.dirname(local_path) or ".", exist_ok=True)
                with fs.open(full, "rb") as src:
                    content = src.read()
                with open(local_path, "wb") as dst:
                    dst.write(content)
                print(f"  {len(content):8d} bytes  {full}  ->  {local_path}")
                extracted += 1

    walk("/")
    print(f"\nDone — {extracted} file(s) extracted to '{OUT_DIR}/'")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <firmware.bin>")
        sys.exit(1)
    extract(sys.argv[1])
