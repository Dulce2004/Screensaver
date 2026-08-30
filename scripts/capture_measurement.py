#!/usr/bin/env python3
"""Captura una ejecución real y breve para el anexo de evidencia."""

from __future__ import annotations

import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "results" / "measurement_capture.log"


def execute(command: list[str]) -> str:
    completed = subprocess.run(
        command,
        cwd=ROOT,
        check=False,
        text=True,
        capture_output=True,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"Falló ({completed.returncode}): {' '.join(command)}\n"
            f"{completed.stdout}\n{completed.stderr}"
        )
    return "$ " + " ".join(command) + "\n" + completed.stdout.strip()


def main() -> int:
    sequential = ROOT / "build" / "BubbleScreensaver.exe"
    openmp = ROOT / "build" / "BubbleScreensaverOpenMP.exe"
    if not sequential.is_file() or not openmp.is_file():
        raise FileNotFoundError("Compile ambos ejecutables antes de capturar")
    sections = [
        execute([
            str(sequential),
            "--benchmark", "10000", "42", "50", "--repeat", "1",
        ]),
        execute([
            str(openmp),
            "--benchmark-parallel", "10000", "42", "50", "4",
            "--repeat", "1",
        ]),
    ]
    OUTPUT.write_text("\n\n".join(sections) + "\n", encoding="utf-8")
    print(f"measurement_log={OUTPUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
