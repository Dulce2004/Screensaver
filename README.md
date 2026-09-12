# Screensaver de burbujas con OpenMP

Proyecto 1 de **Computación Paralela y Distribuida** (UVG, Semestre 2 de 2026).
Implementa un screensaver en C++17 con una versión secuencial y otra paralela
con OpenMP, renderizadas mediante OpenGL, GLFW y GLEW.

**C++17 · OpenMP · OpenGL 3.3 · GLFW · GLEW · PowerShell · Python**

## Descripción

El programa recibe una cantidad `N` de burbujas, genera un estado reproducible
y simula movimiento, rebotes contra los bordes y colisiones elásticas. La
ventana utiliza un canvas de 800 × 600, muestra los FPS en el título y dibuja
las burbujas sobre una textura de fondo.

Además del modo visual, el proyecto incluye:

- benchmark del kernel físico sin renderizado;
- medición de speedup y eficiencia para 1, 2, 4 y 8 hilos;
- campaña visual controlada para comparar FPS;
- validación determinista mediante checksums;
- pruebas automatizadas y evidencia reproducible en CSV.

## Estructura del proyecto

```text
Screensaver/
├── include/bubbles/        Interfaces públicas y tipos compartidos
├── src/                    CLI, física, OpenMP, renderizado y medición
├── tests/                  Pruebas C++ y análisis en Python
├── scripts/                Compilación, pruebas, campañas y limpieza
├── results/                CSV primarios, resúmenes, capturas y gráficas
│   └── figures/            Figuras generadas por los analizadores
├── docs/                   Informe y documentación técnica
│   ├── Proyecto1-Paralela.docx
│   ├── Proyecto1-Paralela.pdf
│   ├── flowchart.md
│   ├── fps-canonical-evidence.md
│   ├── function-catalog.md
│   └── test-log.md
├── data/raw/Fondo.jpg      Textura utilizada por el modo visual
├── third_party/stb_image.h Carga de la imagen de fondo
├── Doxyfile                Configuración de documentación de la API
└── build/                  Binarios locales generados; no se entrega
```

## Requisitos

La configuración utilizada para las mediciones fue Windows 11 AMD64, MSYS2
UCRT64 y GCC 16.2.0.

Desde una terminal MSYS2 UCRT64:

```bash
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-glew
```

También se requiere Python 3 con `matplotlib` para analizar las campañas. En
PowerShell, agregue `C:\msys64\ucrt64\bin` al `PATH` antes de compilar.

## Inicio rápido

Ejecute los comandos desde la raíz del proyecto:

```powershell
.\scripts\build.ps1
.\scripts\run_tests.ps1
```

La compilación crea dos ejecutables en `build/`:

| Ejecutable | Implementación |
| --- | --- |
| `BubbleScreensaver.exe` | Secuencial |
| `BubbleScreensaverOpenMP.exe` | Paralela con OpenMP |

Ambas variantes se compilan con `-Wall -Wextra -Wpedantic -Werror`; la bandera
`-fopenmp` se habilita únicamente para la variante paralela.

## Modos de ejecución

| Modo | Propósito |
| --- | --- |
| Visual | Ejecutar el screensaver hasta cerrar la ventana |
| `--benchmark` | Medir únicamente la física secuencial |
| `--benchmark-parallel` | Medir la física con OpenMP |
| `--fps-supplementary` | Medir la experiencia visual en una ventana controlada |

```text
build\BubbleScreensaver.exe <N> [seed]
build\BubbleScreensaver.exe --benchmark <N> <seed> <steps> [--csv <archivo>] [--repeat <i>]
build\BubbleScreensaverOpenMP.exe --benchmark-parallel <N> <seed> <steps> <threads> [--csv <archivo>] [--repeat <i>]
build\BubbleScreensaver[OpenMP].exe --fps-supplementary <N> <seed> <threads> <vsync:on|off> <warmup_s> <measurement_s> --csv <archivo> --repeat <i>
```

Ejemplos:

```powershell
.\build\BubbleScreensaver.exe 1000 42
.\build\BubbleScreensaverOpenMP.exe 10000 42
.\build\BubbleScreensaver.exe --benchmark 10000 42 500
.\build\BubbleScreensaverOpenMP.exe --benchmark-parallel 10000 42 500 4
```

El modo visual debe iniciarse desde la raíz porque carga la ruta relativa
`data/raw/Fondo.jpg`.

## Diseño paralelo

La simulación utiliza una rejilla uniforme para limitar la búsqueda de
colisiones a la celda de cada burbuja y sus ocho vecinas. En la variante
OpenMP:

1. `omp for` distribuye el movimiento y la detección de pares.
2. Cada hilo conserva una lista local de colisiones.
3. Una región `critical` combina las listas.
4. Una barrera garantiza que la lista compartida esté completa.
5. Una región `single` ordena y resuelve los contactos de forma determinista.

Las variantes secuencial y paralela reutilizan su espacio de trabajo durante
todo el benchmark y producen el mismo checksum final para una configuración
equivalente.

## Validación defensiva

- `1 <= N <= 100000`.
- `1 <= steps <= 10000000` y `N * steps <= 1000000000`.
- `1 <= threads <= 256`, sin superar los procesadores disponibles.
- Rechazo de texto sobrante, enteros negativos y overflow.
- Validación del encabezado CSV y de configuraciones FPS duplicadas.
- Rechazo de mediciones si la ventana se minimiza, cambia de tamaño o se cierra.

## Reproducir la evidencia

### Benchmark físico

La campaña canónica ejecuta 150 mediciones: tres tamaños de problema, una
configuración secuencial, OpenMP con 1, 2, 4 y 8 hilos, y diez repeticiones por
configuración.

```powershell
py .\scripts\run_benchmark_campaign.py --overwrite
py .\scripts\analyze_benchmarks.py
```

### Campaña de FPS

La campaña visual ejecuta 60 mediciones con tres tamaños, Secuencial 1T y
OpenMP 4T, diez repeticiones, VSync activado, 1 s de calentamiento y 3 s de
medición.

```powershell
py .\scripts\run_fps_campaign.py --overwrite
py .\scripts\analyze_fps.py
py .\scripts\capture_measurement.py
```

Para compilar, probar y reproducir ambas campañas:

```powershell
.\scripts\run_all.ps1 -Overwrite
```

Los datos primarios se conservan en `results/benchmark_raw.csv` y
`results/fps_raw.csv`. Los analizadores generan resúmenes y gráficas sin
eliminar valores atípicos. Los FPS se reportan como evidencia de experiencia
visual, no como sustituto del speedup del kernel.

## Resultados destacados

Resultados de la campaña física conservada actualmente en `results/`:

| N | Mejor configuración OpenMP medida | Speedup | Eficiencia |
| ---: | ---: | ---: | ---: |
| 1,000 | OpenMP 1T | 0.990× | 0.990 |
| 10,000 | OpenMP 8T | 2.526× | 0.316 |
| 50,000 | OpenMP 8T | 4.265× | 0.533 |

Los 30 grupos comparables de la campaña finalizaron con checksums coincidentes
entre la versión secuencial y todas las configuraciones OpenMP.

## Documentación

| Archivo | Contenido |
| --- | --- |
| [Proyecto1-Paralela.pdf](docs/Proyecto1-Paralela.pdf) | Informe en formato de entrega |
| [flowchart.md](docs/flowchart.md) | Fuente del diagrama de flujo |
| [function-catalog.md](docs/function-catalog.md) | Catálogo auditable de funciones y tipos |
| [test-log.md](docs/test-log.md) | Bitácora y evidencia de pruebas |
| [fps-canonical-evidence.md](docs/fps-canonical-evidence.md) | Evidencia canónica de la campaña visual |

### Generar la referencia Doxygen

La API pública, los tipos compartidos y los contratos internos relevantes
incluyen comentarios compatibles con Doxygen. Con Doxygen y Graphviz
disponibles en el `PATH`, genere el sitio HTML mediante:

```powershell
doxygen Doxyfile
Start-Process .\build\doxygen\html\index.html
```

La salida se crea dentro de `build/doxygen/` y puede regenerarse en cualquier
momento.

El código, los scripts y la evidencia permiten regenerar los binarios cuando
sea necesario.
