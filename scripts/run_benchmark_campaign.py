#!/usr/bin/env python3
"""Ejecuta la matriz canónica del benchmark físico en orden serial."""

from __future__ import annotations

import argparse
import os
import platform
import subprocess
import sys
from datetime import datetime
from pathlib import Path


DEFAULT_CASES = ((1_000, 5_000), (10_000, 500), (50_000, 100))


def _run(command: list[str]) -> None:
    completed = subprocess.run(command, check=False, text=True, capture_output=True)
    if completed.returncode != 0:
        raise RuntimeError(
            f"Falló ({completed.returncode}): {' '.join(command)}\n"
            f"stdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )


def _write_environment(path: Path) -> None:
    compiler = subprocess.run(
        ["g++", "--version"], check=False, text=True, capture_output=True
    ).stdout.splitlines()
    lines = [
        f"campaign_timestamp={datetime.now().astimezone().isoformat(timespec='seconds')}",
        f"platform={platform.platform()}",
        f"machine={platform.machine()}",
        f"logical_cpu_count={os.cpu_count()}",
        f"python={sys.version.split()[0]}",
        f"compiler={compiler[0] if compiler else 'unavailable'}",
        "seed=42",
        "dt_seconds=1/60",
        "execution_order=serial",
        "raw_outliers=preserved",
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--sequential", type=Path, default=Path("build/BubbleScreensaver.exe"))
    parser.add_argument("--openmp", type=Path, default=Path("build/BubbleScreensaverOpenMP.exe"))
    parser.add_argument("--output", type=Path, default=Path("results/benchmark_raw.csv"))
    parser.add_argument("--environment", type=Path, default=Path("results/environment.txt"))
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--repeats", type=int, default=10)
    parser.add_argument("--threads", type=int, nargs="+", default=[1, 2, 4, 8])
    parser.add_argument("--overwrite", action="store_true")
    args = parser.parse_args()

    for executable in (args.sequential, args.openmp):
        if not executable.is_file():
            raise FileNotFoundError(f"No existe el ejecutable: {executable}")
    if args.output.exists():
        if not args.overwrite:
            raise FileExistsError(f"Ya existe {args.output}; use --overwrite")
        args.output.unlink()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    _write_environment(args.environment)

    total = len(DEFAULT_CASES) * args.repeats * (1 + len(args.threads))
    completed = 0
    for n, steps in DEFAULT_CASES:
        for repeat in range(1, args.repeats + 1):
            _run([
                str(args.sequential), "--benchmark", str(n), str(args.seed),
                str(steps), "--csv", str(args.output), "--repeat", str(repeat),
            ])
            completed += 1
            print(f"[{completed}/{total}] sequential N={n} repeat={repeat}", flush=True)
            for threads in args.threads:
                _run([
                    str(args.openmp), "--benchmark-parallel", str(n), str(args.seed),
                    str(steps), str(threads), "--csv", str(args.output),
                    "--repeat", str(repeat),
                ])
                completed += 1
                print(
                    f"[{completed}/{total}] openmp {threads}T N={n} repeat={repeat}",
                    flush=True,
                )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
