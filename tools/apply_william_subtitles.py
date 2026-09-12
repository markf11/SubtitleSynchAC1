"""Add verified Hungarian William corridor entries without changing existing text.

Usage: python apply_william_subtitles.py subtitles.json [--apply]
The companion william-corridor.hu.json patch must be next to this script in a
release package, or under assets/patches in the source checkout.
"""
import argparse
import json
from datetime import datetime
from pathlib import Path

def merge(data, additions):
    original = json.loads(data.decode('utf-8-sig'))
    conflicts = [key for key in additions if key in original and original[key] != additions[key]]
    if conflicts:
        raise ValueError('Existing entries conflict; nothing changed: ' + ', '.join(conflicts))
    missing = {key: value for key, value in additions.items() if key not in original}
    if not missing:
        return data
    end = data.rfind(b'}')
    body = json.dumps(missing, ensure_ascii=False, indent=2)[1:-1].strip().encode('utf-8')
    updated = data[:end].rstrip() + (b',' if original else b'') + b'\n  ' + body + b'\n' + data[end:]
    parsed = json.loads(updated.decode('utf-8-sig'))
    if parsed != original | missing:
        raise ValueError('Patch validation failed; nothing changed.')
    return updated

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('database', type=Path)
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    patch = Path(__file__).with_name('william-corridor.hu.json')
    if not patch.exists():
        patch = Path(__file__).resolve().parents[1] / 'assets/patches/william-corridor.hu.json'
    additions = json.loads(patch.read_text(encoding='utf-8'))
    data = args.database.read_bytes()
    updated = merge(data, additions)
    if updated == data:
        print('Already patched.')
    elif not args.apply:
        print('Ready to add missing William subtitles; existing entries remain unchanged.')
    else:
        backup = args.database.with_name(args.database.name + '.before-corridor-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f') + '.bak')
        backup.write_bytes(data)
        args.database.write_bytes(updated)
        print('William subtitles added. Backup:', backup)
