"""Audit Limiter's registered byte patterns against a Minecraft PE image."""
import json
import pathlib
import re
import struct
import sys

root = pathlib.Path(__file__).resolve().parents[1]
binary = pathlib.Path(sys.argv[1]).read_bytes()
pe = struct.unpack_from("<I", binary, 0x3C)[0]
section_count = struct.unpack_from("<H", binary, pe + 6)[0]
optional_size = struct.unpack_from("<H", binary, pe + 20)[0]
sections = pe + 24 + optional_size
for index in range(section_count):
    offset = sections + index * 40
    if binary[offset:offset + 8].rstrip(b"\0") == b".text":
        rva, size, raw = struct.unpack_from("<III", binary, offset + 12)
        code = binary[raw:raw + size]
        break
else:
    raise RuntimeError("No .text section")


def scan_pattern(pattern):
    tokens = [None if "?" in item else int(item, 16) for item in pattern.split()]
    segments = []
    start = 0
    while start < len(tokens):
        end = start
        while end < len(tokens) and tokens[end] is not None:
            end += 1
        if end > start:
            segments.append((start, bytes(tokens[start:end])))
        start = end + 1
    anchor_offset, anchor = max(segments, key=lambda item: len(item[1]))
    found = []
    cursor = 0
    while (hit := code.find(anchor, cursor)) >= 0:
        cursor = hit + 1
        base = hit - anchor_offset
        if base >= 0 and base + len(tokens) <= len(code) and all(
            value is None or code[base + i] == value for i, value in enumerate(tokens)
        ):
            found.append(hex(rva + base))
    return found


registry = root / "src" / "Memory" / "Sig" / "SigInit.cpp"
source = registry.read_text(encoding="utf-8-sig")
results = []
for match in re.finditer(r'ADD_SIG\("([^"]+)",\s*"([0-9a-fA-F? ]+)"\)', source):
    name, pattern = match.groups()
    results.append({
        "name": name,
        "line": source.count("\n", 0, match.start()) + 1,
        "pattern": pattern,
        "matches": scan_pattern(pattern),
    })

output = root / "out" / "minecraft-signatures.json"
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(results, indent=2), encoding="utf-8")
for result in results:
    if len(result["matches"]) != 1:
        print(f"{registry.relative_to(root)}:{result['line']}: "
              f"{len(result['matches'])} matches {result['matches'][:5]}")
print(f"{len(results)} patterns: "
      f"{sum(len(r['matches']) == 1 for r in results)} unique, "
      f"{sum(not r['matches'] for r in results)} missing; report: {output}")
