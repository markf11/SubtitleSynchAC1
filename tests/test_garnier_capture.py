import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import review_garnier_capture as garnier


def row(kind, tick, **fields):
    return {'kind': kind, 'tick_ms': tick, 'session_ms': tick, 'fields': fields}


class GarnierReview(unittest.TestCase):
    def session(self, rows):
        return {'session': 1, 'metadata': {'target': 'garnier-confession'}, 'rows': rows}

    def test_early_replacement_identifies_unseen_segment(self):
        identifier = '0x20b40022'
        rows = [row('subtitle_start', 100, id=identifier, text='First <wait=2> Last <duration=4>', db_duration_ms='4000'),
                row('subtitle_segment_start', 100, id=identifier, segment='0', text='First'),
                row('subtitle_end', 1100, id=identifier, playback_elapsed_ms='1000', reason='replaced-by-audio'),
                row('session_end', 1200)]
        report = garnier.analyse(self.session(rows), {}, {'lines': []})
        self.assertEqual(report['subtitle_episodes'][0]['not_observed_segments'], [1])
        self.assertEqual({issue['kind'] for issue in report['issues']},
                         {'segments-not-observed', 'ended-before-database-duration'})

    def test_visible_time_excludes_menu_and_keeps_replay_separate(self):
        identifier = '0x20b40022'
        rows = [row('subtitle_start', 100, id=identifier, text='Line'),
                row('display_state', 100, active_id=identifier, visible='1', paused='0'),
                row('display_state', 300, active_id=identifier, visible='0', paused='1'),
                row('display_state', 800, active_id=identifier, visible='1', paused='0'),
                row('subtitle_end', 1000, id=identifier, reason='duration-expired'),
                row('display_state', 1000, active_id='0x00000000', visible='0'),
                row('subtitle_start', 1100, id=identifier, text='Line'),
                row('display_state', 1100, active_id=identifier, visible='1', paused='0'),
                row('subtitle_end', 1300, id=identifier, reason='duration-expired'),
                row('session_end', 1300)]
        report = garnier.analyse(self.session(rows), {}, {'lines': []})
        self.assertEqual([episode['observed_visible_ms'] for episode in report['subtitle_episodes']], [400, 200])

    def test_unknown_audio_and_false_repeat_markers_never_assign_reference_ids(self):
        rows = [row('audio_dispatch', 100, id='0x20abcdef', raw='0x10abcdef', mapping='missing', resolution='1'),
                row('audio_dispatch', 110, id='0x00000000', raw='0x10b40099', mapping='missing', resolution='2'),
                row('review_marker_f2', 120, marker='1', active_id='0x00000000'),
                row('review_marker_f2', 130, marker='2', active_id='0x00000000'),
                row('queue_dropped', 140, count='3'), row('session_end', 150)]
        reference = {'lines': [{'id': None, 'speaker': 'Garnier', 'text': 'Unmatched'}]}
        report = garnier.analyse(self.session(rows), {}, reference)
        self.assertIsNone(report['scene_reference'][0]['id'])
        self.assertEqual(report['unresolved_audio'][0]['fields']['raw'], '0x10b40099')
        self.assertEqual(report['queue_dropped_events'], 3)
        self.assertEqual(report['f2_groups'][0]['markers'], [1, 2])
        self.assertEqual([candidate['id'] for candidate in report['f2_groups'][0]['candidates']], ['0x20abcdef'])

    def test_capture_ending_mid_line_is_incomplete_not_expired(self):
        report = garnier.analyse(self.session([row('subtitle_start', 100, id='x', text='Line')]), {}, {'lines': []})
        self.assertFalse(report['capture_closed'])
        self.assertFalse(report['subtitle_episodes'][0]['visible_duration_complete'])
        self.assertEqual(report['issues'], [])

    def test_append_sessions_are_not_merged(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'capture.tsv'
            path.write_text('session\t100\t0\ttarget=garnier-confession\n'
                            'audio_dispatch\t110\t10\tid=first\n'
                            'session\t200\t0\ttarget=garnier-confession\n'
                            'audio_dispatch\t210\t10\tid=second\n', encoding='utf-8')
            sessions = garnier.read_sessions(path)
            self.assertEqual(len(sessions), 2)
            self.assertEqual(sessions[1]['rows'][1]['fields']['id'], 'second')
            self.assertEqual(len(sessions[1]['rows']), 2)


if __name__ == '__main__':
    unittest.main()
