"""Read-only verification. Requires pefile. Never writes game binaries."""
import argparse
import hashlib
import json
import pathlib
import re
import struct
import pefile

parser = argparse.ArgumentParser()
parser.add_argument("game", type=pathlib.Path)
args = parser.parse_args()
header = (pathlib.Path(__file__).resolve().parents[1] / "include/hooks/audio_pattern.h").read_text()
pattern = re.search(r'Pattern\s*=\s*"([^"]+)"', header).group(1)
regex = re.compile(b"".join(b"." if t == "??" else re.escape(bytes.fromhex(t)) for t in pattern.split()), re.DOTALL)
call_offset = int(re.search(r'CallOffset\s*=\s*(\d+)', header).group(1))
checked = 0
for name in ("AssassinsCreed_Dx9.exe", "AssassinsCreed_Dx10.exe"):
    path = args.game / name
    if not path.is_file():
        continue
    checked += 1
    data = path.read_bytes()
    pe = pefile.PE(data=data)
    matches = []
    for section in pe.sections:
        if section.Characteristics & 0x20000000:
            for m in regex.finditer(section.get_data()[:section.Misc_VirtualSize]):
                rva = section.VirtualAddress + m.start() + call_offset
                call = pe.get_data(rva, 5)
                target = pe.OPTIONAL_HEADER.ImageBase + rva + 5 + struct.unpack_from("<i", call, 1)[0]
                matches.append({"rva": hex(rva), "call_target": hex(target)})
    print(json.dumps({"exe": name, "sha256": hashlib.sha256(data).hexdigest(), "matches": matches}))
    assert len(matches) == 1, "audio pattern must have exactly one executable-section match"
assert checked, "no supported game executable found"

root = pathlib.Path(__file__).resolve().parents[1]
original = json.loads((root / "assets/subtitles.json").read_text(encoding="utf-8-sig"))
local = json.loads((args.game / "scripts/subtitles.json").read_text(encoding="utf-8-sig"))
assert original.keys() == local.keys(), "subtitle ID sets differ"
tags = re.compile(r"<(?:wait|duration)=[^>]+>")
differences = [k for k in original if tags.findall(original[k]) != tags.findall(local[k])]
print(json.dumps({"subtitle_entries": len(local), "timing_differences": differences}))
