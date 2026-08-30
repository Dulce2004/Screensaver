import math
import unittest

from scripts.analyze_benchmarks import summarize_rows, validate_checksums
from scripts.analyze_fps import summarize_fps_rows


def benchmark_row(implementation, threads, repeat, total_ms, checksum="0xAB"):
    return {
        "implementation": implementation,
        "threads": str(threads),
        "n": "1000",
        "seed": "42",
        "steps": "5000",
        "delta_time_s": "0.016666666666666666",
        "repeat": str(repeat),
        "total_ms": str(total_ms),
        "average_step_ms": str(total_ms / 5000.0),
        "ns_per_element_step": str(total_ms * 1_000_000.0 / 5_000_000.0),
        "initial_checksum": "0x01",
        "final_checksum": checksum,
    }


class BenchmarkAnalysisTests(unittest.TestCase):
    def test_summary_uses_sample_deviation_speedup_and_efficiency(self):
        rows = []
        for repeat, milliseconds in enumerate((8.0, 10.0, 12.0), start=1):
            rows.append(benchmark_row("sequential", 1, repeat, milliseconds))
        for repeat, milliseconds in enumerate((4.0, 5.0, 6.0), start=1):
            rows.append(benchmark_row("parallel", 2, repeat, milliseconds))

        summary = summarize_rows(rows, expected_repeats=3)
        sequential = next(row for row in summary if row["implementation"] == "sequential")
        parallel = next(row for row in summary if row["implementation"] == "parallel")

        self.assertEqual(sequential["repetitions"], 3)
        self.assertAlmostEqual(sequential["mean_ms"], 10.0)
        self.assertAlmostEqual(sequential["stddev_ms"], 2.0)
        self.assertAlmostEqual(parallel["speedup"], 2.0)
        self.assertAlmostEqual(parallel["efficiency"], 1.0)

    def test_checksum_validation_rejects_a_mismatch(self):
        rows = [
            benchmark_row("sequential", 1, 1, 10.0),
            benchmark_row("parallel", 2, 1, 5.0, checksum="0xCD"),
        ]
        with self.assertRaisesRegex(ValueError, "checksum"):
            validate_checksums(rows, expected_repeats=1)


class FpsAnalysisTests(unittest.TestCase):
    def test_fps_summary_is_descriptive_not_speedup(self):
        rows = []
        for repeat, fps in enumerate((58.0, 60.0, 62.0), start=1):
            rows.append(
                {
                    "implementation": "sequential",
                    "threads": "1",
                    "n": "1000",
                    "seed": "42",
                    "repeat": str(repeat),
                    "vsync": "on",
                    "average_fps": str(fps),
                    "average_frame_ms": str(1000.0 / fps),
                    "minimum_interval_fps": str(fps - 2.0),
                    "p95_frame_ms": "18.0",
                    "intervals_below_60_percent": "33.333333",
                }
            )

        summary = summarize_fps_rows(rows, expected_repeats=3)
        self.assertEqual(len(summary), 1)
        self.assertAlmostEqual(summary[0]["mean_fps"], 60.0)
        self.assertTrue(math.isclose(summary[0]["stddev_fps"], 2.0))
        self.assertNotIn("speedup", summary[0])


if __name__ == "__main__":
    unittest.main()
