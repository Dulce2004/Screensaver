#!/usr/bin/env python3
"""Valida, resume y grafica la campaña del benchmark físico."""

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


def _normalize(row: dict[str, str]) -> dict[str, object]:
    return {
        "implementation": _field(row, "implementation"),
        "threads": int(_field(row, "threads")),
        "n": int(_field(row, "N", "n")),
        "seed": int(_field(row, "seed")),
        "steps": int(_field(row, "steps")),
        "dt_seconds": float(_field(row, "dt_seconds", "delta_time_s")),
        "repeat": int(_field(row, "repeat")),
        "total_ms": float(_field(row, "total_ms")),
        "ns_per_element_step": float(_field(row, "ns_per_element_step")),
        "initial_checksum": _field(row, "initial_checksum"),
        "final_checksum": _field(row, "final_checksum"),
    }


def _require_repetitions(
    groups: dict[tuple[object, ...], list[dict[str, object]]],
    expected_repeats: int,
) -> None:
    expected = set(range(1, expected_repeats + 1))
    for key, rows in groups.items():
        actual = {int(row["repeat"]) for row in rows}
        if len(rows) != expected_repeats or actual != expected:
            raise ValueError(
                f"Repeticiones inválidas para {key}: "
                f"esperadas {sorted(expected)}, obtenidas {sorted(actual)}"
            )


def summarize_rows(
    raw_rows: Iterable[dict[str, str]], expected_repeats: int = 10
) -> list[dict[str, object]]:
    rows = [_normalize(row) for row in raw_rows]
    groups: dict[tuple[object, ...], list[dict[str, object]]] = defaultdict(list)
    for row in rows:
        key = (
            row["n"],
            row["steps"],
            row["seed"],
            row["dt_seconds"],
            row["implementation"],
            row["threads"],
        )
        groups[key].append(row)
    if not groups:
        raise ValueError("El CSV de benchmark no contiene filas")
    _require_repetitions(groups, expected_repeats)

    sequential_means: dict[tuple[int, int, int, float], float] = {}
    for key, group in groups.items():
        n, steps, seed, dt_seconds, implementation, threads = key
        if implementation == "sequential" and threads == 1:
            sequential_means[(n, steps, seed, dt_seconds)] = statistics.fmean(
                float(row["total_ms"]) for row in group
            )

    summary: list[dict[str, object]] = []
    for key in sorted(groups, key=lambda item: (item[0], str(item[4]), item[5])):
        n, steps, seed, dt_seconds, implementation, threads = key
        reference_key = (n, steps, seed, dt_seconds)
        if reference_key not in sequential_means:
            raise ValueError(f"Falta la referencia secuencial para N={n}, steps={steps}")
        group = groups[key]
        times = [float(row["total_ms"]) for row in group]
        per_element = [float(row["ns_per_element_step"]) for row in group]
        mean_ms = statistics.fmean(times)
        speedup = sequential_means[reference_key] / mean_ms
        summary.append(
            {
                "implementation": implementation,
                "threads": threads,
                "N": n,
                "seed": seed,
                "steps": steps,
                "dt_seconds": dt_seconds,
                "repetitions": len(group),
                "mean_ms": mean_ms,
                "stddev_ms": statistics.stdev(times) if len(times) > 1 else 0.0,
                "min_ms": min(times),
                "max_ms": max(times),
                "mean_ns_per_element_step": statistics.fmean(per_element),
                "speedup": speedup,
                "efficiency": speedup / int(threads),
            }
        )
    return summary


def validate_checksums(
    raw_rows: Iterable[dict[str, str]], expected_repeats: int = 10
) -> list[dict[str, object]]:
    rows = [_normalize(row) for row in raw_rows]
    configurations: dict[tuple[object, ...], list[dict[str, object]]] = defaultdict(list)
    comparisons: dict[tuple[int, int, int, int], list[dict[str, object]]] = defaultdict(list)
    for row in rows:
        configurations[
            (
                row["n"], row["steps"], row["seed"], row["dt_seconds"],
                row["implementation"], row["threads"],
            )
        ].append(row)
        comparisons[
            (int(row["n"]), int(row["steps"]), int(row["seed"]), int(row["repeat"]))
        ].append(row)
    _require_repetitions(configurations, expected_repeats)

    validation: list[dict[str, object]] = []
    for key in sorted(comparisons):
        group = comparisons[key]
        initial = {str(row["initial_checksum"]) for row in group}
        final = {str(row["final_checksum"]) for row in group}
        if len(initial) != 1 or len(final) != 1:
            raise ValueError(f"Diferencia de checksum en N/steps/seed/repeat={key}")
        validation.append(
            {
                "N": key[0],
                "steps": key[1],
                "seed": key[2],
                "repeat": key[3],
                "configurations": len(group),
                "initial_checksum": next(iter(initial)),
                "final_checksum": next(iter(final)),
                "status": "MATCH",
            }
        )
    return validation


def _read_csv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as stream:
        return list(csv.DictReader(stream))


def _write_csv(path: Path, rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def _create_charts(summary: list[dict[str, object]], figures: Path) -> None:
    import matplotlib.pyplot as plt

    figures.mkdir(parents=True, exist_ok=True)
    sizes = sorted({int(row["N"]) for row in summary})
    for metric, ylabel, filename in (
        ("mean_ms", "Tiempo medio (ms)", "benchmark_time.png"),
        ("speedup", "Speedup S(p)", "benchmark_speedup.png"),
        ("efficiency", "Eficiencia E(p)", "benchmark_efficiency.png"),
    ):
        fig, axis = plt.subplots(figsize=(7.2, 4.2), constrained_layout=True)
        for n in sizes:
            parallel = sorted(
                (
                    row for row in summary
                    if int(row["N"]) == n and row["implementation"] == "parallel"
                ),
                key=lambda row: int(row["threads"]),
            )
            x = [int(row["threads"]) for row in parallel]
            y = [float(row[metric]) for row in parallel]
            axis.plot(x, y, marker="o", linewidth=2, label=f"N={n:,}")
        if metric == "speedup":
            threads = sorted({
                int(row["threads"]) for row in summary
                if row["implementation"] == "parallel"
            })
            axis.plot(threads, threads, "--", color="#777777", label="Ideal")
        if metric == "efficiency":
            axis.axhline(1.0, linestyle="--", color="#777777", label="Ideal")
        axis.set_xlabel("Hilos OpenMP")
        axis.set_ylabel(ylabel)
        axis.set_xticks(sorted({
            int(row["threads"]) for row in summary
            if row["implementation"] == "parallel"
        }))
        axis.grid(True, alpha=0.25)
        axis.legend()
        fig.savefig(figures / filename, dpi=180)
        plt.close(fig)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=Path("results/benchmark_raw.csv"))
    parser.add_argument("--summary", type=Path, default=Path("results/benchmark_summary.csv"))
    parser.add_argument("--checksums", type=Path, default=Path("results/checksum_validation.csv"))
    parser.add_argument("--figures", type=Path, default=Path("results/figures"))
    parser.add_argument("--repeats", type=int, default=10)
    args = parser.parse_args()

    raw = _read_csv(args.input)
    summary = summarize_rows(raw, args.repeats)
    checksums = validate_checksums(raw, args.repeats)
    _write_csv(args.summary, summary)
    _write_csv(args.checksums, checksums)
    _create_charts(summary, args.figures)
    print(f"benchmark_rows={len(raw)} summary_rows={len(summary)} checksum_groups={len(checksums)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
