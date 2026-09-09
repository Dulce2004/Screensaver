# Anexo 1 — Diagrama de flujo

El programa no solicita datos de manera interactiva: recibe `N` y los demás
parámetros por línea de comandos. Si falta un dato obligatorio o un valor no
cumple los límites, muestra el error, imprime el uso y termina sin crear la
ventana ni modificar un CSV.

```mermaid
flowchart TD
    A[Inicio: argc y argv] --> B{¿Argumentos válidos?}
    B -- No --> C[Mostrar error y uso]
    C --> Z[Terminar con código 2]
    B -- Sí --> D{Modo solicitado}
    D -- help --> E[Mostrar uso]
    D -- visual --> F[Generar N burbujas con seed]
    D -- benchmark --> G[Generar estado inicial y checksum]
    D -- benchmark-parallel --> H[Validar hilos OpenMP]
    D -- fps-supplementary --> I[Validar CSV, hilos, VSync y duraciones]
    H --> G
    G --> J[Iniciar reloj sin OpenGL]
    J --> K{Secuencial u OpenMP}
    K -- Secuencial --> L[Región persistente secuencial]
    K -- OpenMP --> M[Región paralela persistente]
    L --> N[Detener reloj y calcular checksum]
    M --> N
    N --> O[Mostrar tiempos; anexar CSV si se pidió]
    O --> Y[Terminar con código 0]
    F --> P[Crear RendererSession RAII]
    I --> P
    P --> Q{¿Ventana y recursos listos?}
    Q -- No --> R[Liberar recursos parciales]
    R --> S[Terminar con código 1]
    Q -- Sí --> T[Actualizar física]
    T --> U[Renderizar fondo y burbujas]
    U --> V[Intercambiar buffers y contar FPS]
    V --> W{¿Fin de medición o cierre?}
    W -- No --> T
    W -- Sí --> X[Liberar OpenGL/GLFW]
    X --> Y
    E --> Y
```

## Paso de física y sincronización

```mermaid
flowchart TD
    A[Calcular subpasos] --> B[omp for: mover burbujas y rebotar en bordes]
    B --> C[single: construir rejilla y limpiar pares]
    C --> D[omp for: detectar pares por celdas vecinas]
    D --> E[critical: reunir vectores locales]
    E --> F[barrier: esperar todos los hilos]
    F --> G{single: ¿hay pares?}
    G -- Sí --> H[Ordenar y resolver impulsos deterministas]
    H --> I{¿Quedan iteraciones?}
    I -- Sí --> C
    G -- No --> J[Salida colectiva del solver]
    I -- No --> K{¿Quedan subpasos/pasos?}
    J --> K
    K -- Sí --> B
    K -- No --> L[Estado final]
```

La sección `critical` protege el vector compartido de pares. La barrera
explícita impide que `single` consuma listas incompletas; la barrera implícita
al terminar `single` hace que todos los hilos observen `collisionPairsFound`
antes de decidir la salida. La resolución se serializa deliberadamente para
evitar escrituras concurrentes sobre una misma burbuja y conservar un orden
determinista idéntico al secuencial.
