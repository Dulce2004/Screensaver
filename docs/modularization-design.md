# Diseño de modularización

## Objetivo

Separar `src/main.cpp` por responsabilidades sin cambiar la interfaz de línea de
comandos, la simulación, los formatos CSV, los resultados numéricos ni los dos
modos de compilación existentes.

## Módulos

- `src/config.hpp`: constantes internas de configuración y límites.
- `types.hpp`: modelos de dominio y estructuras de opciones/resultados.
- `cli.cpp`: ayuda, validación y análisis de argumentos.
- `generation.cpp`: generación reproducible y checksums.
- `csv.cpp`: persistencia y validación de resultados.
- `physics.cpp`: física y colisiones secuenciales, además de auxiliares comunes.
- `physics_openmp.cpp`: distribución y sincronización OpenMP.
- `renderer.cpp`: GLFW, GLEW, OpenGL, shaders, textura y recursos gráficos.
- `benchmark.cpp`: ejecución del benchmark no gráfico.
- `visual.cpp`: bucle interactivo y campaña suplementaria de FPS.
- `main.cpp`: captura de excepciones y despacho por modo.

## Dirección de dependencias

```text
main -> cli, modes
modes/benchmark -> generation, physics, csv
modes/visual -> generation, physics, csv, renderer
physics_openmp -> physics_detail
```

Los encabezados de `include/bubbles/` forman el contrato público. Tanto
`config.hpp` como `physics_detail.hpp` permanecen en `src/` porque son detalles
de implementación. Ningún módulo de física, CSV o CLI depende de OpenGL.

`RendererSession` posee de forma exclusiva la sesión gráfica y libera GLFW y
los recursos OpenGL por RAII, incluso si un modo visual retorna antes o propaga
una excepción. Solo `visual.cpp` crea una sesión; `main.cpp` no conoce recursos
gráficos.

## Restricciones

- Conservar C++17 y las bibliotecas GLFW, GLEW, OpenGL y OpenMP.
- Mantener `BUBBLES_ENABLE_OPENMP` como selector de la variante paralela.
- Mantener exactamente los argumentos, códigos de salida y encabezados CSV.
- La versión secuencial debe compilar sin enlazar OpenMP.
- OpenGL debe permanecer fuera del benchmark de física.
- No modificar todavía el algoritmo ni los resultados experimentales.
