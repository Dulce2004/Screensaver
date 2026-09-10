# Evidencia FPS canónica

Esta campaña complementa el benchmark físico: mide la experiencia completa
(física, dibujo, intercambio de buffers y eventos), por lo que sus cocientes
no se presentan como speedup del algoritmo.

## Protocolo

- Seed 42; canvas lógico fijo de 800×600.
- VSync solicitado: ON.
- Calentamiento: 1 s; medición: 3 s.
- Diez ventanas independientes por configuración.
- El programa cancela una fila si la ventana cambia de tamaño, se minimiza o
  se cierra durante la fase medida.
- Variantes: Secuencial 1T y OpenMP 4T.

## Resultados

| N | Variante | FPS medio | Desv. FPS | Rango FPS | p95 medio (ms) | Intervalos <60 FPS |
|---:|---|---:|---:|---:|---:|---:|
| 1,000 | Secuencial 1T | 240.00 | 0.02 | 239.98–240.06 | 4.76 | 0% |
| 1,000 | OpenMP 4T | 240.01 | 0.04 | 239.96–240.06 | 4.74 | 0% |
| 10,000 | Secuencial 1T | 40.13 | 3.65 | 34.95–47.22 | 56.13 | 88% |
| 10,000 | OpenMP 4T | 154.30 | 2.86 | 148.98–158.98 | 8.26 | 0% |
| 25,000 | Secuencial 1T | 6.95 | 0.08 | 6.83–7.08 | 179.22 | 100% |
| 25,000 | OpenMP 4T | 28.74 | 1.05 | 26.96–30.28 | 55.43 | 100% |

En N=1,000 ambas variantes quedan limitadas cerca de 240 Hz, coherente con el
VSync y el refresco registrado; por ello esa fila no revela capacidad de
cómputo. En N=10,000, OpenMP mantiene más de 60 FPS en todos los intervalos
de 0.5 s medidos, mientras el secuencial no. En N=25,000 ambos quedan debajo de
60 FPS, aunque OpenMP conserva una experiencia claramente más fluida.

## Limitaciones

Los FPS dependen del GPU, controlador, compositor, monitor y carga del sistema.
No sustituyen el benchmark del kernel ni permiten atribuir toda diferencia a
OpenMP. La duración de 3 s responde a una prueba académica controlada; una
evaluación de producción debería usar sesiones más largas y más equipos.
