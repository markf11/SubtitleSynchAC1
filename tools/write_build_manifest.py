"""Create an ASI-only artifact with source and build provenance; never install."""
import argparse
import hashlib
import json
import platform
import shutil
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT, text=True).strip()


def version(command):
    try:
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, check=False)
        return (result.stdout or result.stderr).strip().splitlines()[:3]
    except FileNotFoundError:
        return ['not found in this shell']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if subprocess.run(['git', 'diff', '--quiet', 'HEAD', '--ignore-submodules=none'], cwd=ROOT).returncode:
        raise SystemExit('Commit the reviewed source before generating a provenance manifest.')
    submodules = git('submodule', 'status', '--recursive').splitlines()
    if any(line.startswith(('-', '+', 'U')) for line in submodules):
        raise SystemExit('Initialize submodules at their recorded commit revisions.')
    binary = args.binary.resolve()
    data = binary.read_bytes()
    if data[:2] != b'MZ':
        raise SystemExit('The selected file is not a Windows PE binary.')
    offset = struct.unpack_from('<I', data, 0x3c)[0]
    if data[offset:offset + 4] != b'PE\0\0' or struct.unpack_from('<H', data, offset + 4)[0] != 0x14c:
        raise SystemExit('The selected mod must be an x86 Windows PE binary.')
    commit = git('rev-parse', 'HEAD')
    tracked = git('ls-files').splitlines()
    hashes = {}
    for name in tracked:
        path = ROOT / name
        if path.is_file() and (name.startswith(('src/', 'include/', 'cmake/')) or name == 'CMakeLists.txt'):
            hashes[name] = hashlib.sha256(path.read_bytes()).hexdigest()
    caches = list(binary.parent.glob('CMakeCache.txt')) + list(binary.parent.parent.glob('CMakeCache.txt'))
    compiler = []
    for cache in caches[:1]:
        for line in cache.read_text(encoding='utf-8').splitlines():
            if line.startswith('CMAKE_CXX_COMPILER:FILEPATH='):
                executable = line.split('=', 1)[1]
                compiler = version([executable, '/Bv' if 'cl.exe' == Path(executable).name.lower() else '--version'])
    manifest = {'source_commit': commit, 'binary': 'SubtitleSynchAC1.asi',
                'binary_sha256': hashlib.sha256(data).hexdigest(), 'architecture': 'x86',
                'source_file_sha256': hashes, 'submodule_commits': submodules,
                'platform': platform.platform(), 'python': platform.python_version(),
                'cmake': version(['cmake', '--version']), 'compiler_version_output': compiler,
                'claim': 'Build provenance and byte identification, not a security certification or bit-for-bit reproducibility claim.'}
    output = args.output.resolve()
    if output == binary.parent or output == ROOT or binary == output / 'SubtitleSynchAC1.asi':
        raise SystemExit('Use a separate artifact directory.')
    output.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(binary, output / 'SubtitleSynchAC1.asi')
    (output / 'SOURCE-COMMIT.txt').write_text(commit + '\n', encoding='ascii')
    (output / 'BUILD-MANIFEST.json').write_text(json.dumps(manifest, ensure_ascii=True, indent=2) + '\n', encoding='utf-8')
    sums = ''.join(f'{hashlib.sha256((output / name).read_bytes()).hexdigest().upper()} *{name}\n'
                   for name in ('SubtitleSynchAC1.asi', 'SOURCE-COMMIT.txt', 'BUILD-MANIFEST.json'))
    (output / 'SHA256SUMS.txt').write_text(sums, encoding='ascii')
    print(f'commit={commit} binary_sha256={manifest["binary_sha256"]} source_files={len(hashes)}')


if __name__ == '__main__':
    main()
