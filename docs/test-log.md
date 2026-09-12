# Anexo 3 — Bitácora de pruebas

## Ambiente

- Sistema: Windows 11, AMD64, 32 procesadores lógicos informados.
- Compilador: GCC MSYS2 UCRT64 16.2.0, C++17 y OpenMP.
- Python: 3.13.14.
- Seed: 42; paso físico: 1/60 s; ejecuciones seriales para evitar interferencia entre procesos.
- Se preservaron todas las observaciones primarias, sin eliminar outliers.


## Pruebas automáticas

Comando:

```powershell
.\scripts\run_tests.ps1
```

Resultado del 22/09/2026:

```text
core_behavior=PASS
initial_penetration=0
head_on_collision=PASS
sequential_openmp_equivalence=PASS
persistent_equivalence=PASS
maximum_parallel_penetration=0
benchmark_fairness=PASS
Ran 3 tests in 0.001s — OK
tests=PASS
```

La compilación secuencial y OpenMP también terminó sin advertencias con
`-Wall -Wextra -Wpedantic -Werror`.

## Benchmark físico canónico

Comandos:

```powershell
py .\scripts\run_benchmark_campaign.py --overwrite
py .\scripts\analyze_benchmarks.py
```

Cada celda contiene 10 mediciones. El speedup usa
`S(p) = media_secuencial / media_paralela(p)` y la eficiencia
`E(p) = S(p) / p`. Los tiempos son del kernel físico, sin creación de ventana,
renderizado ni generación inicial.

| N | Pasos | Variante | Hilos | Media (ms) | Desv. (ms) | Speedup | Eficiencia |
|---:|---:|---|---:|---:|---:|---:|---:|
| 1,000 | 5,000 | Secuencial | 1 | 1,948.66 | 4.96 | 1.000 | 1.000 |
| 1,000 | 5,000 | OpenMP | 1 | 1,969.30 | 7.37 | 0.990 | 0.990 |
| 1,000 | 5,000 | OpenMP | 2 | 2,597.55 | 47.56 | 0.750 | 0.375 |
| 1,000 | 5,000 | OpenMP | 4 | 3,460.35 | 37.21 | 0.563 | 0.141 |
| 1,000 | 5,000 | OpenMP | 8 | 6,059.97 | 82.24 | 0.322 | 0.040 |
| 10,000 | 500 | Secuencial | 1 | 7,114.39 | 44.26 | 1.000 | 1.000 |
| 10,000 | 500 | OpenMP | 1 | 7,200.90 | 31.86 | 0.988 | 0.988 |
| 10,000 | 500 | OpenMP | 2 | 4,351.89 | 57.73 | 1.635 | 0.817 |
| 10,000 | 500 | OpenMP | 4 | 2,878.76 | 74.80 | 2.471 | 0.618 |
| 10,000 | 500 | OpenMP | 8 | 2,816.86 | 161.87 | 2.526 | 0.316 |
| 50,000 | 100 | Secuencial | 1 | 12,158.86 | 40.01 | 1.000 | 1.000 |
| 50,000 | 100 | OpenMP | 1 | 12,328.13 | 21.83 | 0.986 | 0.986 |
| 50,000 | 100 | OpenMP | 2 | 6,841.18 | 59.81 | 1.777 | 0.889 |
| 50,000 | 100 | OpenMP | 4 | 4,111.50 | 32.88 | 2.957 | 0.739 |
| 50,000 | 100 | OpenMP | 8 | 2,851.04 | 44.71 | 4.265 | 0.533 |

El CSV primario contiene 150 filas y 15 configuraciones. La validación contiene
30 grupos (tres tamaños por diez repeticiones); las cinco variantes de cada
grupo comparten checksum inicial y final. Resultado: 30 `MATCH`, 0 diferencias.

## Evidencia visual suplementaria

Comandos:

```powershell
py .\scripts\run_fps_campaign.py --overwrite
py .\scripts\analyze_fps.py
```

Se obtuvieron 60 filas: N={1,000; 10,000; 25,000}, Secuencial 1T y OpenMP
4T, 10 repeticiones, VSync ON, 1 s de calentamiento y 3 s de medición. Los
resultados y sus limitaciones se detallan en `docs/fps-canonical-evidence.md`.

## Archivos de evidencia

- `results/benchmark_raw.csv`: tiempos primarios, identidad y checksums.
- `results/benchmark_summary.csv`: medias, dispersión, speedup y eficiencia.
- `results/checksum_validation.csv`: equivalencia por repetición.
- `results/fps_raw.csv`: mediciones visuales primarias.
- `results/fps_summary.csv`: resumen descriptivo de FPS.
- `results/figures/`: gráficos regenerables desde los CSV.
