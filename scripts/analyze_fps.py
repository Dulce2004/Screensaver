#!/usr/bin/env python3
"""Resume la campaña visual suplementaria sin interpretarla como speedup."""

from __future__ import annotations

import argparse
import csv
import statistics
from collections import defaultdict
from pathlib import Path
from typing import Iterable


def _field(row: dict[str, str], *names: str) -> str:
    for name in names:
        if name in row:
            return row[name]
    raise ValueError(f"Falta la columna requerida: {'/'.join(names)}")


def summarize_fps_rows(
    raw_rows: Iterable[dict[str, str]], expected_repeats: int = 10
) -> list[dict[str, object]]:
    groups: dict[tuple[object, ...], list[dict[str, str]]] = defaultdict(list)
    for row in raw_rows:
        key = (
            _field(row, "implementation"),
            int(_field(row, "threads")),
            int(_field(row, "N", "n")),
            int(_field(row, "seed")),
            _field(row, "vsync"),
        )
        groups[key].append(row)
    if not groups:
        raise ValueError("El CSV FPS no contiene filas")

    summary: list[dict[str, object]] = []
    expected = set(range(1, expected_repeats + 1))
    for key in sorted(groups, key=lambda item: (item[2], str(item[0]), item[1])):
        implementation, threads, n, seed, vsync = key
        rows = groups[key]
        repeats = {int(_field(row, "repeat")) for row in rows}
        if len(rows) != expected_repeats or repeats != expected:
            raise ValueError(f"Repeticiones FPS inválidas para {key}: {sorted(repeats)}")
        fps = [float(_field(row, "average_fps")) for row in rows]
        frame_ms = [float(_field(row, "average_frame_ms")) for row in rows]
        minimum_fps = [float(_field(row, "minimum_interval_fps")) for row in rows]
        p95_ms = [float(_field(row, "p95_frame_ms")) for row in rows]
        below = [float(_field(row, "intervals_below_60_percent")) for row in rows]
        summary.append(
            {
                "implementation": implementation,
                "threads": threads,
                "N": n,
                "seed": seed,
                "vsync": vsync,
                "repetitions": len(rows),
                "mean_fps": statistics.fmean(fps),
                "stddev_fps": statistics.stdev(fps) if len(fps) > 1 else 0.0,
                "min_fps": min(fps),
                "max_fps": max(fps),
                "mean_frame_ms": statistics.fmean(frame_ms),
                "mean_minimum_interval_fps": statistics.fmean(minimum_fps),
                "mean_p95_frame_ms": statistics.fmean(p95_ms),
                "mean_intervals_below_60_percent": statistics.fmean(below),
            }
        )
    return summary


def _create_chart(summary: list[dict[str, object]], output: Path) -> None:
    import matplotlib.pyplot as plt

    labels = [
        f"{row['implementation']} {row['threads']}T\nN={int(row['N']):,}"
        for row in summary
    ]
    means = [float(row["mean_fps"]) for row in summary]
    errors = [float(row["stddev_fps"]) for row in summary]
    colors = ["#2b6cb0" if row["implementation"] == "sequential" else "#38a169" for row in summary]
    fig, axis = plt.subplots(figsize=(9.0, 4.5), constrained_layout=True)
    axis.bar(labels, means, yerr=errors, capsize=4, color=colors)
    axis.axhline(60.0, linestyle="--", color="#c53030", label="60 FPS")
    axis.set_ylabel("FPS medio (± desviación estándar)")
    axis.tick_params(axis="x", labelrotation=25)
    axis.grid(True, axis="y", alpha=0.25)
    axis.legend()
    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=180)
    plt.close(fig)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=Path("results/fps_raw.csv"))
    parser.add_argument("--summary", type=Path, default=Path("results/fps_summary.csv"))
    parser.add_argument("--figure", type=Path, default=Path("results/figures/fps_summary.png"))
    parser.add_argument("--repeats", type=int, default=10)
    args = parser.parse_args()

    with args.input.open("r", encoding="utf-8", newline="") as stream:
        summary = summarize_fps_rows(list(csv.DictReader(stream)), args.repeats)
    args.summary.parent.mkdir(parents=True, exist_ok=True)
    with args.summary.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(summary[0]))
        writer.writeheader()
        writer.writerows(summary)
    _create_chart(summary, args.figure)
    print(f"fps_summary_rows={len(summary)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
