"""List audio IDs near F2 markers in a SubtitleSynchAC1.capture.tsv file.

Usage: python tools/find_f2_candidates.py path/to/SubtitleSynchAC1.capture.tsv
"""

import argparse
import csv
import json
from pathlib import Path


def read_rows(path):
    session = 0
    rows = []
    for line in path.read_text(encoding="utf-8-sig", errors="replace").splitlines():
        if not line or line.startswith("#"):
            continue
        parts = line.split("\t")
        if parts[0] == "session":
            session += 1
            continue
        if len(parts) < 4 or parts[0] not in {
            "audio_dispatch", "review_marker_f2", "subtitle_start", "subtitle_end"}:
            continue
        try:
            tick = int(parts[1])
        except ValueError:
            continue
        fields = dict(item.split("=", 1) for item in parts[3:] if "=" in item)
        fields["_session_ms"] = parts[2]
        rows.append((session, tick, parts[0], fields))
    return rows


def review_groups(rows, before_ms=4000, after_ms=1000):
    """Keep repeat F2 presses in one subtitle episode; never infer a transcript."""
    episodes = {}
    groups = []
    sounds = {}
    for session, tick, kind, fields in rows:
        if kind == "audio_dispatch":
            sounds.setdefault(session, []).append((tick, fields))
        elif kind == "subtitle_start":
            episodes[session] = episodes.get(session, 0) + 1
        elif kind == "review_marker_f2":
            active = fields.get("active_id", "0x00000000")
            key = (session, episodes.get(session, 0), active)
            last = groups[-1] if groups else None
            # Missing subtitles have no episode: group only rapid repeat presses.
            repeat = last is not None and last["key"] == key and (
                active != "0x00000000" or tick - last["ticks"][-1] <= 2000)
            if not repeat:
                last = {"key": key, "session": session, "active_id": active,
                        "text": fields.get("text", ""), "markers": [], "ticks": [],
                        "session_ms": []}
                groups.append(last)
            last["markers"].append(int(fields.get("marker", 0)))
            last["ticks"].append(tick)
            last["session_ms"].append(int(fields.get("_session_ms", 0)))
    # Candidate IDs are distinct sounds, not a claim that any is speech.
    for group in groups:
        candidates = {}
        for tick, sound in sounds.get(group["session"], []):
            deltas = [tick - marker for marker in group["ticks"]]
            nearby = [delta for delta in deltas if -before_ms <= delta <= after_ms]
            if not nearby:
                continue
            identifier = sound.get("id", "?")
            if identifier == "0x00000000":
                continue  # Unresolved dispatch is not a replayable resource ID.
            delta = min(nearby, key=abs)
            old = candidates.get(identifier)
            if old is None or abs(delta) < abs(old["nearest_ms"]):
                candidates[identifier] = {"id": identifier, "nearest_ms": delta,
                                         "mapping": sound.get("mapping", "?")}
        group["candidates"] = sorted(candidates.values(), key=lambda item: abs(item["nearest_ms"]))
        del group["key"]
    return groups


def write_review(path, rows, groups):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.with_suffix(".json").write_text(json.dumps(groups, ensure_ascii=False, indent=2), encoding="utf-8")
    # TSV is a video-review index. Keep all candidate IDs; false presses are
    # for a human to reject, never for the tool to turn into database entries.
    with path.with_suffix(".tsv").open("w", encoding="utf-8-sig", newline="") as file:
        writer = csv.writer(file, delimiter="\t")
        writer.writerow(["session", "F2_markers", "session_seconds", "tick_ms", "active_id", "subtitle_text",
                         "missing_candidates", "all_candidates", "review_status"])
        for group in groups:
            candidates = group["candidates"]
            describe = lambda item: f'{item["id"]} ({item["nearest_ms"]:+d}ms)'
            writer.writerow([group["session"], ",".join(map(str, group["markers"])),
                             ",".join(f"{ms / 1000:.3f}" for ms in group["session_ms"]),
                             ",".join(map(str, group["ticks"])), group["active_id"], group["text"],
                             "; ".join(map(describe, [c for c in candidates if c["mapping"] == "missing"])),
                             "; ".join(map(describe, candidates)), "needs video/audio confirmation"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("--marker", type=int, help="Show one numbered marker per session")
    parser.add_argument("--session", type=int, action="append", help="Include only these capture sessions")
    parser.add_argument("--review-output", type=Path, help="Write a grouped JSON/TSV review index")
    parser.add_argument("--before-ms", type=int, default=4000)
    parser.add_argument("--after-ms", type=int, default=1000)
    args = parser.parse_args()
    rows = read_rows(args.capture)
    if args.session:
        rows = [row for row in rows if row[0] in args.session]
    if args.review_output:
        groups = review_groups(rows, args.before_ms, args.after_ms)
        write_review(args.review_output, rows, groups)
        markers = sum(len(group["markers"]) for group in groups)
        print(f"markers={markers} review_groups={len(groups)} sessions={len({r[0] for r in rows})}")
        return
    sounds = [row for row in rows if row[2] == "audio_dispatch"]
    for session, tick, _, fields in rows:
        if _ != "review_marker_f2":
            continue
        marker = int(fields.get("marker", "0"))
        if args.marker is not None and args.marker != marker:
            continue
        print(f"session={session} marker={marker} tick={tick} active={fields.get('active_id', '?')}")
        nearby = [row for row in sounds if row[0] == session and
                  tick - args.before_ms <= row[1] <= tick + args.after_ms]
        nearby.sort(key=lambda row: (abs(row[1] - tick), row[1]))
        for _, sound_tick, _, sound in nearby:
            print(f"  {sound_tick - tick:+6d} ms  id={sound.get('id', '?')}  "
                  f"raw={sound.get('raw', '?')}  mapping={sound.get('mapping', '?')}")
        if not nearby:
            print("  (no observed audio dispatch in this window)")


if __name__ == "__main__":
    main()
