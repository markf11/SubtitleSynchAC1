"""Normalize semicolons, overlong ellipses and repeated exclamation marks.
Usage: python normalize_hu_subtitle_punctuation.py subtitles.json [--apply]
"""
import argparse
import json
import re
from datetime import datetime
from pathlib import Path

def normalize(data):
    before = json.loads(data.decode('utf-8-sig'))
    result = re.sub(rb'!{2,}', b'!', re.sub(rb'\.{4,}', b'...', data.replace(b';', b',')))
    expected = {
        k: re.sub(r'!{2,}', '!', re.sub(r'\.{4,}', '...', v.replace(';', ',')))
        for k, v in before.items()
    }
    if json.loads(result.decode('utf-8-sig')) != expected:
        raise ValueError('Unexpected JSON representation; no changes made.')
    return result

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('database', type=Path)
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    data = args.database.read_bytes()
    result = normalize(data)
    print('Semicolons to replace:', data.count(b';'))
    print('Overlong ellipses:', len(re.findall(rb'\.{4,}', data)))
    print('Repeated exclamation marks:', len(re.findall(rb'!{2,}', data)))
    if args.apply and result != data:
        backup = args.database.with_name(args.database.name + '.before-punctuation-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f') + '.bak')
        backup.write_bytes(data)
        args.database.write_bytes(result)
        print('Updated. Backup:', backup)
