import importlib.util
from pathlib import Path
import unittest

MODULE = Path(__file__).resolve().parents[1] / "tools" / "find_f2_candidates.py"
spec = importlib.util.spec_from_file_location("f2", MODULE)
f2 = importlib.util.module_from_spec(spec)
spec.loader.exec_module(f2)


class MarkerGrouping(unittest.TestCase):
    def test_same_line_repeat_and_replay_are_separate(self):
        def marker(session, tick, number, active):
            return session, tick, "review_marker_f2", {"marker": str(number), "active_id": active}
        rows = [
            (1, 100, "subtitle_start", {"id": "0x20abcdef"}),
            (1, 100, "audio_dispatch", {"id": "0x20abcdef", "mapping": "primary"}),
            marker(1, 500, 1, "0x20abcdef"), marker(1, 1500, 2, "0x20abcdef"),
            (1, 2000, "subtitle_start", {"id": "0x20abcdef"}),
            marker(1, 2300, 3, "0x20abcdef"),
            marker(2, 500, 1, "0x20abcdef"),
        ]
        groups = f2.review_groups(rows)
        self.assertEqual([g["markers"] for g in groups], [[1, 2], [3], [1]])
        self.assertEqual(groups[-1]["candidates"], [])

    def test_unmapped_markers_and_unresolved_audio_are_not_merged_blindly(self):
        rows = [(1, 50, "audio_dispatch", {"id": "0x00000000", "mapping": "missing"})]
        for tick, marker in [(100, 1), (200, 2), (3000, 3)]:
            rows.append((1, tick, "review_marker_f2", {
                "marker": str(marker), "active_id": "0x00000000"}))
        groups = f2.review_groups(rows)
        self.assertEqual([g["markers"] for g in groups], [[1, 2], [3]])
        self.assertTrue(all(g["candidates"] == [] for g in groups))


if __name__ == "__main__":
    unittest.main()
