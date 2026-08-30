#!/usr/bin/env python3
"""Ejecuta la evidencia visual suplementaria en ventanas separadas."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def _run(command: list[str]) -> None:
    completed = subprocess.run(command, check=False, text=True, capture_output=True)
    if completed.returncode != 0:
        raise RuntimeError(
            f"Falló ({completed.returncode}): {' '.join(command)}\n"
            f"stdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--sequential", type=Path, default=Path("build/BubbleScreensaver.exe"))
    parser.add_argument("--openmp", type=Path, default=Path("build/BubbleScreensaverOpenMP.exe"))
    parser.add_argument("--output", type=Path, default=Path("results/fps_raw.csv"))
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--repeats", type=int, default=10)
    parser.add_argument("--threads", type=int, default=4)
    parser.add_argument("--warmup", type=int, default=1)
    parser.add_argument("--measurement", type=int, default=3)
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

    sizes = (1_000, 10_000, 25_000)
    total = len(sizes) * args.repeats * 2
    completed = 0
    for n in sizes:
        for repeat in range(1, args.repeats + 1):
            common = [
                "--fps-supplementary", str(n), str(args.seed), "1", "on",
                str(args.warmup), str(args.measurement), "--csv", str(args.output),
                "--repeat", str(repeat),
            ]
            _run([str(args.sequential), *common])
            completed += 1
            print(f"[{completed}/{total}] sequential N={n} repeat={repeat}", flush=True)

            common[3] = str(args.threads)
            _run([str(args.openmp), *common])
            completed += 1
            print(
                f"[{completed}/{total}] openmp {args.threads}T N={n} repeat={repeat}",
                flush=True,
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
