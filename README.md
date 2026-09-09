# Screensaver de burbujas — Secuencial y OpenMP

Proyecto de Computación Paralela y Distribuida (UVG, Semestre 2, 2026),
implementado en C++17 con OpenGL/GLFW/GLEW y OpenMP.

El programa recibe `N`, genera burbujas reproducibles de varios colores y las
dibuja sobre un fondo JPEG. Las burbujas rebotan en los bordes y entre sí; el
título muestra FPS. También incluye un benchmark sin OpenGL para calcular
speedup y eficiencia de la física con una comparación determinista.

## Estructura

- `include/bubbles/`: contratos públicos y tipos.
- `src/`: CLI, generación, física, OpenMP, CSV, renderizador y modos.
- `tests/`: pruebas de comportamiento, colisiones, equivalencia y análisis.
- `scripts/`: compilación, pruebas, campañas, análisis, informe y limpieza.
- `results/`: evidencia primaria, resúmenes y figuras reproducibles.
- `docs/`: diseño y fuentes auditables de los anexos.
- `data/raw/Fondo.jpg`: textura del fondo.
- `third_party/stb_image.h`: cargador de imágenes de Sean Barrett.
- `Informe_Final.pdf`: copia de entrega del informe actualizado.
- `output/pdf/Informe_Final.pdf`: salida canónica del generador.

## Requisitos

Configuración probada: Windows 11, MSYS2 UCRT64, GCC 16.2.0.

```bash
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-glew
```

Agregue `C:\msys64\ucrt64\bin` al `PATH`. Los scripts de análisis e informe
usan Python 3, matplotlib y ReportLab; Poppler permite verificar el PDF.

## Compilar y probar

Desde PowerShell, en la raíz:

```powershell
.\scripts\build.ps1
.\scripts\run_tests.ps1
```

Los binarios se crean únicamente en `build/`. Ambas variantes usan
`-Wall -Wextra -Wpedantic -Werror`; OpenMP se habilita solo en la paralela.

## Uso

```text
build\BubbleScreensaver.exe <N> [seed]
build\BubbleScreensaver.exe --benchmark <N> <seed> <steps> [--csv <file>] [--repeat <i>]
build\BubbleScreensaverOpenMP.exe --benchmark-parallel <N> <seed> <steps> <threads> [--csv <file>] [--repeat <i>]
build\BubbleScreensaver[OpenMP].exe --fps-supplementary <N> <seed> <threads> <vsync:on|off> <warmup_s> <measurement_s> --csv <file> --repeat <i>
```

Ejemplos:

```powershell
.\build\BubbleScreensaver.exe 1000 42
.\build\BubbleScreensaverOpenMP.exe 10000 42
.\build\BubbleScreensaver.exe --benchmark 10000 42 500
.\build\BubbleScreensaverOpenMP.exe --benchmark-parallel 10000 42 500 4
```

Ejecute el modo visual desde la raíz porque la textura usa la ruta relativa
`data/raw/Fondo.jpg`.

## Validación defensiva

- `1 <= N <= 100000`.
- `1 <= steps <= 10000000` y `N * steps <= 1000000000`.
- `1 <= threads <= 256`, sin superar procesadores disponibles.
- Enteros sin signo, sin texto sobrante ni overflow.
- CSV con encabezado compatible y sin duplicados en la campaña FPS.
- Ventana de medición FPS fija, visible y sin solicitud de cierre.

## Diseño paralelo

Una rejilla uniforme limita la detección a celdas vecinas. OpenMP reparte el
movimiento y la detección mediante `omp for`; cada hilo acumula pares en un
vector local. Una sección `critical` los reúne, una barrera espera a todos y
`single` ordena y resuelve contactos, evitando carreras sobre las burbujas.
La salida cuando ya no existen pares es colectiva. Secuencial y OpenMP
reutilizan su workspace durante todo el benchmark y producen el mismo checksum.

## Reproducir resultados

La campaña física ejecuta 150 mediciones: tres tamaños, Secuencial 1T y OpenMP
1/2/4/8T, diez repeticiones por configuración.

```powershell
py .\scripts\run_benchmark_campaign.py --overwrite
py .\scripts\analyze_benchmarks.py
```

La campaña visual ejecuta 60 ventanas: tres tamaños, Secuencial 1T y OpenMP
4T, diez repeticiones, VSync ON, 1 s de calentamiento y 3 s medidos.

```powershell
py .\scripts\run_fps_campaign.py --overwrite
py .\scripts\analyze_fps.py
py .\scripts\capture_measurement.py
py .\scripts\build_report.py
```

Para ejecutar el flujo completo:

```powershell
.\scripts\run_all.ps1 -Overwrite
```

Los datos primarios permanecen en `results/benchmark_raw.csv` y
`results/fps_raw.csv`; los analizadores crean resúmenes y figuras. No se
eliminan valores atípicos. Los FPS son evidencia suplementaria de experiencia,
no el speedup del kernel.

## Limpiar ejecutables

El entregable no debe contener `.exe`:

```powershell
.\scripts\clean_generated.ps1
```

El script elimina ejecutables de la raíz y de `build/`; el código y la evidencia
permiten regenerarlos.

## Procedencia y pendientes

`stb_image.h` declara licencia MIT/dominio público. La procedencia exacta de
`data/raw/Fondo.jpg` no quedó registrada y se trata como limitación; antes de
una publicación externa conviene reemplazarla por un recurso propio o con
licencia documentada.

El enlace y el historial del repositorio se incorporarán al final, cuando el
equipo indique el repositorio real. No se inventó historial Git.
