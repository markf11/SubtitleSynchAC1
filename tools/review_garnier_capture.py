"""Review a focused Garnier capture without guessing spoken text or audio IDs.

Usage: python tools/review_garnier_capture.py capture.tsv --database subtitles.json
Writes a JSON evidence report and a readable Markdown report beside the capture.
"""
import argparse
import json
import re
from pathlib import Path

from find_f2_candidates import review_groups

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / 'localization/garnier-confession-reference.json'
TAGS = re.compile(r'<(?:wait|duration)=[^>]+>')


def read_sessions(path):
    sessions = []
    current = None
    for number, line in enumerate(path.read_text(encoding='utf-8-sig', errors='replace').splitlines(), 1):
        parts = line.split('\t')
        if not line or line.startswith('#') or len(parts) < 3:
            continue
        try:
            tick, relative = int(parts[1]), int(parts[2])
        except ValueError:
            continue
        fields = dict(item.split('=', 1) for item in parts[3:] if '=' in item)
        if parts[0] == 'session':
            current = {'session': len(sessions) + 1, 'start_ms': tick, 'metadata': fields, 'rows': []}
            sessions.append(current)
        if current is not None:
            current['rows'].append({'kind': parts[0], 'tick_ms': tick,
                                    'session_ms': relative, 'fields': fields, 'file_line': number})
    return sessions


def segments(value):
    return [part.strip() for part in re.split(r'<wait=[^>]+>', re.sub(r'<duration=[^>]+>', '', value)) if part.strip()]


def analyse(session, database, reference):
    rows = session['rows']
    episodes = []
    active = None
    display = None
    display_intervals = []
    for row in rows:
        fields, tick = row['fields'], row['tick_ms']
        kind = row['kind']
        if kind == 'subtitle_start':
            active = {'id': fields.get('id'), 'start_ms': tick, 'end_ms': None,
                      'start_fields': fields, 'segments_started': [], 'segments_skipped': [],
                      'end_reason': 'capture-ended-without-subtitle-end'}
            episodes.append(active)
        elif kind in ('subtitle_segment_start', 'subtitle_segment_skipped') and active and fields.get('id') == active['id']:
            key = 'segments_started' if kind == 'subtitle_segment_start' else 'segments_skipped'
            active[key].append({'index': int(fields.get('segment', '-1')), 'at_ms': tick,
                                'playback_ms': float(fields.get('playback_elapsed_ms', 0)), 'text': fields.get('text', '')})
        elif kind == 'subtitle_end' and active and fields.get('id') == active['id']:
            active.update(end_ms=tick, end_fields=fields, end_reason=fields.get('reason', '?'))
            active = None
        if kind in ('display_state', 'session_end'):
            if display:
                display['end_ms'] = tick
                display_intervals.append(display)
                display = None
            if kind == 'display_state' and fields.get('visible') == '1' and fields.get('paused') != '1':
                display = {'id': fields.get('active_id'), 'start_ms': tick, 'text': fields.get('text', '')}

    issues = []
    for episode in episodes:
        raw = episode['start_fields'].get('text', database.get(episode['id'], ''))
        expected = segments(raw)
        seen = {part['index'] for part in episode['segments_started']}
        episode['expected_segments'] = expected
        episode['not_observed_segments'] = [index for index in range(len(expected)) if index not in seen]
        end = episode['end_ms']
        episode['observed_visible_ms'] = sum(
            max(0, min(interval['end_ms'], end) - max(interval['start_ms'], episode['start_ms']))
            for interval in display_intervals if end is not None and interval['id'] == episode['id'])
        episode['visible_duration_complete'] = end is not None and any(row['kind'] == 'session_end' for row in rows)
        end_fields = episode.get('end_fields', {})
        elapsed = float(end_fields.get('playback_elapsed_ms', 0))
        duration = float(episode['start_fields'].get('db_duration_ms', 0))
        if end is not None and episode['not_observed_segments']:
            issues.append({'id': episode['id'], 'kind': 'segments-not-observed',
                           'indices': episode['not_observed_segments'], 'start_ms': episode['start_ms']})
        if end is not None and duration > 0 and elapsed + 200 < duration:
            issues.append({'id': episode['id'], 'kind': 'ended-before-database-duration',
                           'reason': episode['end_reason'], 'playback_ms': elapsed, 'db_duration_ms': duration})
        if episode['segments_skipped']:
            issues.append({'id': episode['id'], 'kind': 'render-update-skipped-segments'})

    audio = [row for row in rows if row['kind'] == 'audio_dispatch']
    unmapped = [row for row in audio if row['fields'].get('mapping') == 'missing']
    unresolved = [row for row in audio if row['fields'].get('resolution') == '2']
    drops = sum(int(row['fields'].get('count', 0)) for row in rows if row['kind'] == 'queue_dropped')
    known = []
    for line in reference['lines']:
        identifier = line.get('id')
        known.append({**line, 'observed_dispatches': sum(row['fields'].get('id') == identifier for row in audio) if identifier else None,
                      'subtitle_starts': sum(episode['id'] == identifier for episode in episodes) if identifier else None})
    marker_rows = [(session['session'], row['tick_ms'], row['kind'],
                    {**row['fields'], '_session_ms': str(row['session_ms'])}) for row in rows]
    return {'session': session['session'], 'metadata': session['metadata'],
            'capture_closed': any(row['kind'] == 'session_end' for row in rows),
            'scene_reference': known, 'subtitle_episodes': episodes, 'issues': issues,
            'unmapped_audio': unmapped, 'unresolved_audio': unresolved, 'queue_dropped_events': drops,
            'f2_groups': review_groups(marker_rows), 'display_intervals': display_intervals,
            'audio_decisions': [row for row in rows if row['kind'] == 'audio_decision'],
            'timeline': rows,
            'limitations': 'A hangindító hook értesítést rögzít, nem a hang tényleges végét vagy tartalmát. A null ID-k párosításához hang/videó szükséges. Hiányzó értesítés vagy korai feliratvég gyanú, nem önmagában bizonyított javítás.'}


def markdown(report):
    lines = ['# Garnier vallomása – célzott diagnosztika', '',
             f"Session: {report['session']}. Lezárt felvétel: {'igen' if report['capture_closed'] else 'nem'}. Eldobott hangesemények: {report['queue_dropped_events']}.", '',
             report['limitations'], '', '## Szövegkönyvi ellenőrzőlista', '',
             '| Beszélő | Azonosító | Hangértesítés / feliratindítás | Mondat |', '|---|---|---|---|']
    for item in report['scene_reference']:
        identifier = item['id'] or 'Párosítandó'
        count = f"{item['observed_dispatches']} / {item['subtitle_starts']}" if item['id'] else 'Hang/videó alapján'
        lines.append(f"| {item['speaker']} | {identifier} | {count} | {item['text']} |")
    lines += ['', '## Feliratok és szövegrészek', '']
    for episode in report['subtitle_episodes']:
        lines += [f"**{episode['id']}** – indult: {episode['start_ms']} ms; vége: {episode['end_ms']}; ok: {episode['end_reason']}; megfigyelt láthatóság: {episode['observed_visible_ms']} ms.", '']
        for part in episode['segments_started']:
            lines.append(f"- Rész {part['index']}: {part['at_ms']} ms, lejátszási idő {part['playback_ms']:.0f} ms – {part['text']}")
        if episode['not_observed_segments']:
            lines += [f"- Nem láttunk indítást ezekhez a részekhez: {episode['not_observed_segments']}."]
        lines += ['']
    lines += ['## Gyanús pontok', '']
    lines += ['- ' + json.dumps(item, ensure_ascii=False) for item in report['issues']] or ['Nem észleltünk szerkezeti problémát; a hang és a szöveg egyezését ez nem igazolja.']
    lines += ['', '## F2-jelölések és közeli hangjelöltek', '']
    for group in report['f2_groups']:
        lines += [f"- F2 {group['markers']}, idő: {group['session_ms']} ms, aktív: {group['active_id']}; szöveg: {group['text']}"]
        for candidate in group['candidates']:
            lines.append(f"  - {candidate['id']} ({candidate['nearest_ms']:+d} ms), adatbázis: {candidate['mapping']}")
    lines += ['', f"Párosítatlan értesítések: {len(report['unmapped_audio'])}; feloldatlan események: {len(report['unresolved_audio'])}. Minden nyers ID és az idővonal a JSON-jelentésben megmarad.", '']
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--database', type=Path, required=True)
    parser.add_argument('--session', type=int, help='One-based session index; defaults to latest Garnier session')
    parser.add_argument('--output', type=Path, help='Report filename prefix')
    args = parser.parse_args()
    sessions = read_sessions(args.capture)
    if not sessions:
        parser.error('No capture session found')
    selected = [session for session in sessions if session['session'] == args.session] if args.session else [
        session for session in sessions if session['metadata'].get('target') == 'garnier-confession']
    if args.session and not selected:
        parser.error('Requested session does not exist')
    session = (selected or sessions)[-1]
    database = json.loads(args.database.read_text(encoding='utf-8-sig'))
    reference = json.loads(REFERENCE.read_text(encoding='utf-8'))
    report = analyse(session, database, reference)
    report['capture_file'] = str(args.capture.resolve())
    report['database_file'] = str(args.database.resolve())
    output = args.output or args.capture.with_suffix('.review')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.with_suffix('.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    output.with_suffix('.md').write_text(markdown(report), encoding='utf-8')
    print(f"session={session['session']} episodes={len(report['subtitle_episodes'])} suspicious={len(report['issues'])} markers={len(report['f2_groups'])}")


if __name__ == '__main__':
    main()
