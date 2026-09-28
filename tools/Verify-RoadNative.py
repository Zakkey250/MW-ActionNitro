"""Emulate the exact target's Traffic lane selector, then check C++ classification.
Reads the locally installed game only; does not run or modify it.
"""
import hashlib
import re
import json
import pathlib
import struct
import subprocess
import sys
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP

root = pathlib.Path(__file__).resolve().parents[1]
game = pathlib.Path(sys.argv[1])
exe = (game / 'speed.exe').read_bytes()
policy = (root / 'src/ExecutableTargets.h').read_text(encoding='utf-8')
allowed = {(int(size), sha) for size, sha in re.findall(r'",(\d+),"([0-9a-f]{64})"', policy)}
assert (len(exe), hashlib.sha256(exe).hexdigest()) in allowed
pe = pefile.PE(data=exe)
blob = (game / 'TRACKS/L2RA.BUN').read_bytes()
header = blob.rfind(b'PRAC', 0, blob.find(b'dnNR'))
tag = blob.find(b'fpNR', header, header + 512)
assert tag >= 0
count, relative = struct.unpack_from('<II', blob, tag + 8)
profiles = blob[tag + relative:tag + relative + count * 64]
assert len(profiles) == count * 64 and count > 0
u = Uc(UC_ARCH_X86, UC_MODE_32)
u.mem_map(0x400000, 0x700000)
u.mem_write(0x400000, pe.get_memory_mapped_image())
u.mem_map(0x1000000, 0x40000)
nav, nodes, segments, pf, output, stub = (0x1000000, 0x1001000, 0x1002000, 0x1003000, 0x1004000, 0x1005000)
pack = lambda x: struct.pack('<I', x)
scale = struct.unpack('<f', pe.get_data(0x88d526 - 0x400000, 4))[0]
assert .001 < scale < .1
u.mem_write(0x9b3a68, struct.pack('<f', scale))
u.mem_write(0x9b38b8, pack(pf) + pack(nodes) + pack(segments))
u.mem_write(nav + 0x80, pack(1))  # eLaneType::Traffic
u.mem_write(nodes + 14, b'\0\0')
u.mem_write(nodes + 32 + 14, b'\0\0')
u.mem_write(segments, struct.pack('<HH', 0, 1))
lines = []
for index in range(count):
 profile = profiles[index * 64:(index + 1) * 64]
 n, divider = profile[:2]
 assert n <= 15 and divider <= n
 packed = struct.unpack_from('<15I', profile, 4)
 u.mem_write(pf, profile)
 for node in (0, 1):
  for flags in (0, 0x200, 0x400, 0x600):
   suffix = bool(node) ^ bool((flags >> (9 if node else 10)) & 1)
   if not any(v & 15 == 1 and ((i >= divider) == suffix) for i, v in enumerate(packed[:n])):
    continue  # Native may intentionally fall back to wrong-way lanes on closed roads.
   u.mem_write(segments + 10, struct.pack('<H', flags))
   # finit; push node, segmentIndex, requestedOffset; call native; store ST(0); hlt
   code = b'\xdb\xe3\x68' + pack(node) + b'\x6a\x00\x6a\x00\xb9' + pack(nav)
   code += b'\xb8' + pack(0x76e340) + b'\xff\xd0\xd9\x1d' + pack(output) + b'\xf4'
   u.mem_write(stub, code)
   u.ctl_remove_cache(stub, stub + len(code))
   u.reg_write(UC_X86_REG_ESP, 0x103f000)
   u.emu_start(stub, stub + len(code), count=10000)
   offset = struct.unpack('<f', u.mem_read(output, 4))[0]
   lines.append(f'{n} {divider} {node} {flags} {offset:.9g} {scale:.9g} ' + ' '.join(f'{v:x}' for v in packed))
result = subprocess.run([str(root / 'bin/RoadTests.exe')], input='\n'.join(lines), text=True, capture_output=True)
print(result.stdout, end='')
print(result.stderr, end='')
assert result.returncode == 0, 'C++ classifier disagrees with native Traffic selector'
report = {'profiles': count, 'nativeSelectedFixtures': len(lines), 'classifierAssertionsPerFixture': 2,
          'scale': scale, 'sourceExeSHA256': hashlib.sha256(exe).hexdigest(),
          'roadBlobSHA256': hashlib.sha256(blob).hexdigest(),
          'nativeSelector': '0076E340', 'runtimeVerified': False, 'result': 'PASS'}
(root / 'evidence/alpha4-road-native.json').write_text(json.dumps(report, indent=2), encoding='utf8')
