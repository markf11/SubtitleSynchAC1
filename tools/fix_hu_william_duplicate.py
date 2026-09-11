"""Apply one verified Hungarian subtitle correction, preserving all other bytes.

Usage: python fix_hu_william_duplicate.py path/to/subtitles.json [--apply]
Without --apply, only reports the proposed change. Creates a backup on apply.
"""
import argparse
import json
from pathlib import Path
from datetime import datetime

KEY = "0x20b50021"
BEFORE = "Akkor hát nem is tartalak fel... kegyelmes uram. <duration=1.51>"
AFTER = "Akkor hát nem is tartalak fel... <duration=1.51>"

def patch(data):
    decoded = data.decode("utf-8-sig")
    db = json.loads(decoded)
    if db.get(KEY) == AFTER:
        return data
    if db.get(KEY) != BEFORE:
        raise ValueError("The expected Hungarian entry was not found; no changes made.")
    old = json.dumps(BEFORE, ensure_ascii=False).encode("utf-8")
    new = json.dumps(AFTER, ensure_ascii=False).encode("utf-8")
    if data.count(old) != 1:
        raise ValueError("Expected one literal entry; no changes made.")
    result = data.replace(old, new, 1)
    updated = json.loads(result.decode("utf-8-sig"))
    assert {k for k in db if db[k] != updated[k]} == {KEY}
    return result

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("database", type=Path)
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    data = args.database.read_bytes()
    result = patch(data)
    if result == data:
        print("Already corrected.")
    elif args.apply:
        backup = args.database.with_name(args.database.name + ".before-william-" + datetime.now().strftime("%Y%m%d-%H%M%S-%f") + ".bak")
        backup.write_bytes(data)
        args.database.write_bytes(result)
        print("Corrected", KEY, "Backup:", backup)
    else:
        print(KEY, "would be corrected; duration and all other entries stay unchanged.")
