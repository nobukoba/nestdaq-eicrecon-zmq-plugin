#!/usr/bin/env python3
"""Compare EICrecon collection counts with a recorded RARiS multipart stream."""
import argparse
import re
import struct
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('capture', type=Path)
p.add_argument('eicrecon_log', type=Path)
args = p.parse_args()
pattern = r'^\[Processor\] event=(\d+) TOF1=(\d+) TOF2=(\d+) TOF3=(\d+)'
rows = re.findall(pattern, args.eicrecon_log.read_text(), re.M)
events = {int(a): tuple(map(int, (b, c, d))) for a, b, c, d in rows}
if len(rows) != len(events):
    raise ValueError('duplicate event number in log')
data = args.capture.read_bytes()
pos, tf, fem, counts, checked = 0, None, None, [0, 0, 0], 0
while pos < len(data):
    if pos + 8 > len(data): raise ValueError('truncated part header')
    length, more = struct.unpack_from('<II', data, pos); pos += 8
    if more not in (0, 1) or pos + length > len(data): raise ValueError('invalid part')
    part = data[pos:pos + length]; pos += length
    if part[:8] == b'TIMEFRM\0':
        tf = struct.unpack_from('<I', part, 16)[0]; counts = [0, 0, 0]
    elif part[:8] == b'SUBTIME\0':
        fem = struct.unpack_from('<I', part, 24)[0]
    elif part[:8] == b'HRTBEAT\0' and fem == 0xc0a80a28:
        for (word,) in struct.iter_unpack('<Q', part[16:]):
            channel = (word >> 51) & 127
            if word >> 58 == 11 and channel < 48: counts[channel // 16] += 1
    if not more:
        if events.get(tf) != tuple(counts): raise ValueError(f'collection mismatch for TF {tf}')
        checked += 1
if checked != 2815 or len(events) != checked or sum(map(sum, events.values())) != 68956:
    raise ValueError('unexpected event or hit count')
print(f'PASS: {checked} TFs; 68956 hits; per-event collection counts match')
