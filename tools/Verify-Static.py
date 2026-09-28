"""Read-only target and hook evidence. Run with the workspace analysis Python."""
import hashlib
import json
import pathlib
import re
import sys
import pefile
import capstone

root = pathlib.Path(__file__).resolve().parents[1]
target = pathlib.Path(sys.argv[1])
data = target.read_bytes()
digest = hashlib.sha256(data).hexdigest()
policy = (root / 'src/ExecutableTargets.h').read_text(encoding='utf-8')
allowed = {(int(size), sha) for size, sha in re.findall(r'",(\d+),"([0-9a-f]{64})"', policy)}
installer = (root / 'tools/Install.ps1').read_text(encoding='utf-8-sig')
installer_allowed = {(int(size), sha.lower()) for sha, size in re.findall(r"'([A-F0-9]{64})'=(\d+)", installer)}
assert len(allowed) == 3 and installer_allowed == allowed, "installer/runtime target policy mismatch"
assert (len(data), digest) in allowed, "unsupported executable"
pe = pefile.PE(data=data)
assert pe.OPTIONAL_HEADER.ImageBase == 0x400000
source = (root / "src/Plugin.cpp").read_text(encoding="utf-8")
checks = []
for address, expected in re.findall(r'Match\((0x[0-9a-f]+),"([0-9a-f]+)"\)', source):
    va = int(address, 16)
    actual = pe.get_data(va - 0x400000, len(expected) // 2).hex()
    assert actual == expected, f"guard mismatch {address}: {actual} != {expected}"
    checks.append({"address": address, "bytes": actual, "pass": True})
result = {"target": str(target), "sha256": digest, "size": len(data), "guards": checks,
          "runtimeVerified": False, "oncomingAdapter": "RNpf-enabled-runtime-pending"}
(root / "evidence/static-verification.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
with (root / "evidence/nos-disassembly.txt").open("w", encoding="utf-8") as out:
    for va, size in [(0x692930, 810), (0x6a0430, 160), (0x442a70, 32), (0x670e50, 32)]:
        for i in md.disasm(pe.get_data(va - 0x400000, size), va):
            out.write(f"{i.address:08X} {i.bytes.hex():32} {i.mnemonic} {i.op_str}\n")
print(f"PASS target SHA-256 and {len(checks)} exact guards; static only")
