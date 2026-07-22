import unittest

from revcount.tracker import AngleTracker, InvalidSample, required_sample_rate


def signed16(value: int) -> int:
    value &= 0xFFFF
    return value - 0x10000 if value >= 0x8000 else value


class AngleTrackerTests(unittest.TestCase):
    def test_four_electrical_revolutions_are_one_mechanical_turn(self):
        tracker = AngleTracker(max_rpm=1000)
        samples = 1025
        for index in range(samples):
            count = round(index * 4 * 65536 / (samples - 1))
            tracker.update(signed16(count), index * 0.001)
        self.assertAlmostEqual(tracker.mechanical_turns, 1.0, places=6)

    def test_reverse_unwrap(self):
        tracker = AngleTracker(max_rpm=1000)
        samples = 1025
        for index in range(samples):
            count = -round(index * 8 * 65536 / (samples - 1))
            tracker.update(signed16(count), index * 0.001)
        self.assertAlmostEqual(tracker.mechanical_turns, -2.0, places=6)

    def test_wrap_boundary_jitter_cancels(self):
        tracker = AngleTracker(max_rpm=1000)
        values = [32760, -32760, 32762, -32758, -32750]
        for index, value in enumerate(values):
            tracker.update(value, index * 0.001)
        self.assertEqual(tracker.accumulated_counts, 26)

    def test_device_timestamp_drop_is_invalid(self):
        tracker = AngleTracker(max_rpm=1000, expected_tick_step=30)
        tracker.update(0, 0.0, 100)
        with self.assertRaisesRegex(InvalidSample, "device sample gap"):
            tracker.update(100, 0.002, 160)

    def test_host_gap_that_can_hide_half_turn_is_invalid(self):
        tracker = AngleTracker(max_rpm=1000)
        tracker.update(0, 0.0)
        with self.assertRaisesRegex(InvalidSample, "sampling gap"):
            tracker.update(100, 0.008)

    def test_exact_half_turn_is_ambiguous(self):
        tracker = AngleTracker(max_rpm=1000)
        tracker.update(0, 0.0)
        with self.assertRaisesRegex(InvalidSample, "ambiguous adjacent"):
            tracker.update(-32768, 0.001)

    def test_required_rate_has_four_x_wrap_margin(self):
        self.assertAlmostEqual(required_sample_rate(500), 133.3333333333)
        self.assertAlmostEqual(required_sample_rate(1000), 266.6666666667)


if __name__ == "__main__":
    unittest.main()
