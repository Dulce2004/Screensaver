# Guía de commits y publicación del proyecto

## Integrantes

- Daniel Chet
- Dulce Ambrosio
- Cristian Tunchez

Esta guía divide el proyecto en 15 commits: cinco por integrante. Cada persona
debe revisar, comprender y realizar personalmente los commits que tiene
asignados, usando su nombre, correo y cuenta reales.

> No usar `--author` para atribuir trabajo a otra persona, no modificar fechas y
> no crear historial retroactivo. Si no existía un repositorio previo, el
> requisito de dos semanas de historial no puede corregirse artificialmente.

## 1. Archivos que formarán parte del repositorio

Se deben versionar:

- `include/`
- `src/`
- `tests/`
- `scripts/`
- `results/`
- `data/raw/Fondo.jpg`, después de verificar su licencia
- `third_party/stb_image.h`
- `README.md`
- `Informe_Final.pdf`
- `output/pdf/Informe_Final.pdf`
- `docs/flowchart.md`
- `docs/function-catalog.md`
- `docs/test-log.md`
- `docs/fps-canonical-evidence.md`
- `docs/modularization-design.md`
- `docs/guia-commits-equipo.md`
- `.gitignore`
- `.gitattributes`

No se deben versionar:

- Ejecutables y la carpeta `build/`.
- Archivos temporales o cachés de Python.
- Skills y configuración local de los agentes.
- El informe anterior guardado en `docs/archive/`.
- Planes internos de corrección o ejecución.
- El PDF del enunciado proporcionado por el curso.

## 2. Crear `.gitignore`

Crear `.gitignore` en la raíz con este contenido:

```gitignore
# Compilación
build/
*.exe

# Temporales y cachés
tmp/
**/__pycache__/
*.py[cod]

# Configuración local de agentes
.agents/
.codex/
skills-lock.json

# Material interno o histórico
docs/archive/
docs/superpowers/
docs/compliance-correction-design.md

# Enunciado del curso: no es parte del código del equipo
Proyecto 1 - Computación Paralela y Distribuida (1).pdf
```

## 3. Crear `.gitattributes`

Crear `.gitattributes` en la raíz con este contenido:

```gitattributes
* text=auto

*.cpp text eol=lf
*.hpp text eol=lf
*.py text eol=lf
*.md text eol=lf
*.csv text eol=lf
*.ps1 text eol=crlf

*.jpg binary
*.png binary
*.pdf binary
```

## 4. Crear el repositorio privado

Crear primero un repositorio vacío y privado en GitHub. No agregar desde
GitHub un README, `.gitignore` o licencia, porque ya se prepararán localmente.

Desde la raíz del proyecto:

```powershell
git init
git branch -M main
git remote add origin https://github.com/ORGANIZACION/REPOSITORIO.git
git remote -v
git status --short
```

Reemplazar `ORGANIZACION/REPOSITORIO` por la dirección real.

## 5. Identidad de cada integrante

Antes de sus commits, cada persona debe configurar su identidad real. No usar
un correo de ejemplo en el repositorio definitivo.

### Daniel Chet

```powershell
git config user.name "Daniel Chet"
git config user.email "CORREO_REAL_DE_DANIEL"
git config --get user.name
git config --get user.email
```

### Dulce Ambrosio

```powershell
git config user.name "Dulce Ambrosio"
git config user.email "CORREO_REAL_DE_DULCE"
git config --get user.name
git config --get user.email
```

### Cristian Tunchez

```powershell
git config user.name "Cristian Tunchez"
git config user.email "CORREO_REAL_DE_CRISTIAN"
git config --get user.name
git config --get user.email
```

Cada integrante debe configurar su identidad inmediatamente antes de realizar
sus commits. Si trabajan en computadoras distintas, cada uno debe hacerlo en su
propia copia del repositorio.

## 6. Secuencia exacta de commits

Antes de cada commit se recomienda ejecutar:

```powershell
git status --short
git diff --cached --stat
git diff --cached
```

No usar `git add .` durante la importación inicial. Los comandos siguientes
agregan únicamente los archivos correspondientes a cada cambio.

### Commit 1 — Daniel: configuración del repositorio

```powershell
git add -- .gitignore .gitattributes
git diff --cached --stat
git commit -m "chore: configure repository and file policies"
```

### Commit 2 — Daniel: tipos compartidos y CLI

```powershell
git add -- `
  include/bubbles/types.hpp `
  include/bubbles/cli.hpp `
  include/bubbles/modes.hpp `
  src/config.hpp `
  src/cli.cpp
git diff --cached --stat
git commit -m "feat: add shared types and validated command line interface"
```

### Commit 3 — Daniel: generación determinista

```powershell
git add -- `
  include/bubbles/generation.hpp `
  src/generation.cpp
git diff --cached --stat
git commit -m "feat: add deterministic bubble generation and checksums"
```

### Commit 4 — Cristian: física secuencial y colisiones

Cristian debe configurar primero su identidad real.

```powershell
git add -- `
  include/bubbles/physics.hpp `
  src/physics.cpp `
  src/physics_detail.hpp
git diff --cached --stat
git commit -m "feat: implement sequential physics and collision grid"
```

### Commit 5 — Cristian: kernel OpenMP

```powershell
git add -- src/physics_openmp.cpp
git diff --cached --stat
git commit -m "feat: add persistent OpenMP physics kernel"
```

### Commit 6 — Cristian: benchmark y CSV

```powershell
git add -- `
  include/bubbles/csv.hpp `
  src/csv.cpp `
  src/benchmark.cpp
git diff --cached --stat
git commit -m "feat: add headless benchmark and CSV persistence"
```

### Commit 7 — Dulce: renderizador OpenGL

Dulce debe configurar primero su identidad real. Antes de este commit, el
equipo debe confirmar la licencia de `data/raw/Fondo.jpg` o reemplazarla por
una imagen propia o con licencia verificable.

```powershell
git add -- `
  include/bubbles/renderer.hpp `
  src/renderer.cpp `
  tests/renderer_contract_test.cpp `
  third_party/stb_image.h `
  data/raw/Fondo.jpg
git diff --cached --stat
git commit -m "feat: add RAII OpenGL renderer and graphical resources"
```

### Commit 8 — Dulce: screensaver visual y medición FPS

```powershell
git add -- src/visual.cpp
git diff --cached --stat
git commit -m "feat: add visual screensaver and controlled FPS mode"
```

### Commit 9 — Daniel: entrada principal y automatización

Daniel debe volver a configurar su identidad real.

```powershell
git add -- `
  src/main.cpp `
  scripts/__init__.py `
  scripts/build.ps1 `
  scripts/run_tests.ps1 `
  scripts/clean_generated.ps1 `
  tests/core_behavior_test.cpp
git diff --cached --stat
git commit -m "build: add application entry point and reproducible test workflow"
```

Después de este commit ya debe ser posible ejecutar:

```powershell
.\scripts\build.ps1
.\scripts\run_tests.ps1
```

### Commit 10 — Cristian: pruebas de colisiones y equivalencia

Cristian debe volver a configurar su identidad real.

```powershell
git add -- `
  tests/collision_test.cpp `
  tests/benchmark_fairness_test.cpp
git diff --cached --stat
git commit -m "test: verify collisions and sequential OpenMP equivalence"
```

Verificación:

```powershell
.\scripts\run_tests.ps1
```

### Commit 11 — Dulce: campañas y análisis

Dulce debe volver a configurar su identidad real.

```powershell
git add -- `
  scripts/run_benchmark_campaign.py `
  scripts/run_fps_campaign.py `
  scripts/analyze_benchmarks.py `
  scripts/analyze_fps.py `
  scripts/capture_measurement.py `
  tests/analysis_test.py
git diff --cached --stat
git commit -m "perf: automate benchmark and FPS campaigns"
```

Verificación:

```powershell
py -m unittest tests.analysis_test -v
```

### Commit 12 — Cristian: resultados del benchmark

```powershell
git add -- `
  results/benchmark_raw.csv `
  results/benchmark_summary.csv `
  results/checksum_validation.csv `
  results/environment.txt `
  results/figures/benchmark_time.png `
  results/figures/benchmark_speedup.png `
  results/figures/benchmark_efficiency.png
git diff --cached --stat
git commit -m "perf: publish canonical benchmark evidence"
```

La evidencia esperada es:

- 150 filas en `benchmark_raw.csv`.
- 15 configuraciones resumidas.
- 30 grupos de checksum con estado `MATCH`.

### Commit 13 — Dulce: resultados FPS

```powershell
git add -- `
  results/fps_raw.csv `
  results/fps_summary.csv `
  results/measurement_capture.log `
  results/figures/fps_summary.png `
  results/figures/measurement_capture.png
git diff --cached --stat
git commit -m "perf: publish canonical FPS evidence"
```

La evidencia esperada es:

- 60 filas en `fps_raw.csv`.
- 6 configuraciones resumidas.
- Diez repeticiones por configuración.

### Commit 14 — Daniel: documentación y flujo completo

Daniel debe volver a configurar su identidad real.

```powershell
git add -- `
  README.md `
  docs/flowchart.md `
  docs/function-catalog.md `
  docs/modularization-design.md `
  docs/guia-commits-equipo.md `
  scripts/run_all.ps1
git diff --cached --stat
git commit -m "docs: document architecture setup and end to end workflow"
```

### Commit 15 — Dulce: informe final y anexos experimentales

Dulce debe volver a configurar su identidad real.

```powershell
git add -- `
  docs/test-log.md `
  docs/fps-canonical-evidence.md `
  scripts/build_report.py `
  Informe_Final.pdf `
  output/pdf/Informe_Final.pdf
git diff --cached --stat
git commit -m "docs: add final report and experimental appendices"
```

## 7. Validación final

Ejecutar antes del primer push:

```powershell
.\scripts\run_tests.ps1
.\scripts\build.ps1
.\scripts\clean_generated.ps1
```

Confirmar que no haya ejecutables:

```powershell
$executables = @(rg --files -g '*.exe')
if ($executables.Count -ne 0) {
    $executables
    throw 'El repositorio todavía contiene ejecutables.'
}
Write-Host 'Validación correcta: no hay ejecutables.'
```

Confirmar que ningún archivo interno se haya agregado por error:

```powershell
git status --short
git ls-files
git diff --check
```

Revisar el historial y la distribución de autores:

```powershell
git log --oneline --graph --decorate --all
git shortlog -sne HEAD
```

Resultado esperado:

- 15 commits de contenido.
- Cinco commits realizados por cada integrante.
- Ningún `.exe` versionado.
- Ningún archivo de `.agents/`, `docs/archive/` o `docs/superpowers/`.
- Informe final y resultados experimentales presentes.
- Árbol de trabajo limpio.

## 8. Publicar el repositorio

Realizar el primer push cuando las validaciones anteriores pasen:

```powershell
git push -u origin main
```

Comprobar el remoto:

```powershell
git remote -v
git status
git log --oneline --graph --decorate -15
```

El repositorio debe permanecer privado hasta el momento indicado por el curso.
Al hacerlo público, revisar nuevamente la licencia de todos los recursos,
especialmente `data/raw/Fondo.jpg`.

## 9. Trabajo colaborativo con ramas

Si cada integrante trabaja desde su propia computadora, puede usar una rama
personal:

```powershell
git switch main
git pull --ff-only origin main
git switch -c nombre/rama
```

Nombres sugeridos:

```text
daniel/core-cli-build
cristian/physics-openmp-benchmark
dulce/renderer-fps-report
```

Después de terminar sus commits:

```powershell
git push -u origin nombre/rama
```

Luego se crea un Pull Request hacia `main`. Si la contribución de un commit fue
realmente conjunta, se pueden agregar trailers `Co-authored-by` con los nombres
y correos reales de quienes participaron. No deben utilizarse únicamente para
distribuir artificialmente la autoría.
