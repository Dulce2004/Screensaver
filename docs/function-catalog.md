# Anexo 2 — Catálogo de funciones y tipos

## Tipos públicos

| Tipo | Campos o interfaz | Salida / responsabilidad |
|---|---|---|
| `Vec2` | `float x`, `float y` | Representa una posición o velocidad bidimensional. |
| `Bubble` | `position`, `velocity`, `radius`, `color[4]` | Estado físico y visual de una burbuja. |
| `ProgramOptions` | modo, `N`, seed, pasos, repetición, hilos, CSV, VSync y duraciones | Resultado validado de la línea de comandos. |
| `BenchmarkResult` | configuración, tiempos y checksums | Fila primaria del benchmark físico. |
| `FpsResult` | configuración de ventana y métricas de cuadros | Fila primaria de la prueba visual suplementaria. |
| `RendererConfig` | `bubbleCount`, `threadCount`, `vsyncEnabled`, `fixedWindowSize` | Configura la sesión gráfica. |
| `RendererDimensions` | ancho/alto lógico y del framebuffer | Permite comprobar que la ventana permanezca estable. |
| `RendererSession` | constructor, `isReady()`, `close()`, destructor | Dueño RAII no copiable de GLFW/OpenGL; libera recursos incluso ante salida temprana. |

## CLI y modos

| Función | Entradas | Salida | Propósito |
|---|---|---|---|
| `parseCommandLine` | `argc: int`, `argv: char**`, `options: ProgramOptions&`, `errorMessage: string&` | `bool` y opciones/error por referencia | Reconoce los cuatro modos; rechaza texto, negativos, overflow, duplicados y límites inválidos. |
| `printUsage` | `output: ostream&` | Ninguna | Imprime sintaxis y límites. |
| `runVisual` | `options: const ProgramOptions&` | Código `int` | Ejecuta el screensaver hasta que se cierre la ventana. |
| `runBenchmark` | `options: const ProgramOptions&` | Código `int` | Mide únicamente el kernel físico, informa tiempos y opcionalmente guarda CSV. |
| `runFpsSupplementary` | `options: const ProgramOptions&` | Código `int` | Ejecuta calentamiento y medición visual controlada; valida estabilidad de la ventana. |
| `main` | `argc: int`, `argv: char**` | Código `int` | Captura excepciones, invoca la CLI y despacha ayuda, benchmark o visual. |

### Auxiliares privados de CLI

| Función | Entradas | Salida | Propósito |
|---|---|---|---|
| `parseUnsigned64` | `text: string_view`, `value: uint64_t&` | `bool` | Convierte un decimal completo con `from_chars`, sin signos ni texto sobrante. |
| `parseBubbleCount` | texto, `bubbleCount`, error | `bool` | Valida el rango permitido de N. |
| `parseSeed` | texto, `seed`, error | `bool` | Valida una semilla de 64 bits sin signo. |
| `parseSteps` | texto, `steps`, error | `bool` | Valida la cantidad de pasos. |
| `parseThreadCount` | texto, hilos, error | `bool` | Valida la cantidad solicitada de hilos. |
| `parseRepeat` | texto, repetición, error | `bool` | Valida el identificador de repetición. |
| `parseFpsPhaseSeconds` | texto, permiso de cero, nombre, segundos, error | `bool` | Valida duraciones de calentamiento y medición. |
| `parseVSync` | texto, `enabled`, error | `bool` | Acepta únicamente `on` u `off`. |
| `createDefaultSeed` | Ninguna | `uint64_t` | Combina reloj y `random_device` para el modo visual sin seed. |

## Generación y persistencia

| Función | Entradas | Salida | Propósito |
|---|---|---|---|
| `generateBubbles` | `bubbleCount: size_t`, `seed: uint64_t`, `areaWidth/Height: float` | `vector<Bubble>` | Genera posiciones no superpuestas, radios adaptados a N, velocidades y colores reproducibles. |
| `checksumBubbleState` | `bubbles: const vector<Bubble>&` | `uint64_t` | Resume todos los campos mediante FNV-1a para comprobar equivalencia. |
| `formatChecksum` | `checksum: uint64_t` | `string` | Produce hexadecimal de 16 dígitos. |
| `appendBenchmarkCsv` | ruta, `BenchmarkResult`, mensaje de error | `bool` | Verifica encabezado y agrega una fila primaria. |
| `validateFpsCsvAppend` | ruta, `FpsResult`, `writeHeader`, error | `bool` | Rechaza esquemas incompatibles y configuraciones repetidas. |
| `appendFpsCsv` | ruta, `FpsResult`, error | `bool` | Agrega una medición FPS y fuerza el vaciado/cierre del archivo. |

### Auxiliares privados de generación y CSV

| Función | Entradas | Salida | Propósito |
|---|---|---|---|
| `hsvToRgb` | matiz, saturación y valor | `array<float, 3>` | Convierte el color pseudoaleatorio a RGB. |
| `appendHashBytes` | hash, valor y cantidad de bytes | Hash modificado | Incorpora una representación entera al FNV-1a. |
| `splitSimpleCsvRow` | fila sin comillas | `vector<string>` | Divide el esquema FPS controlado por comas. |
| `isSameFpsConfiguration` | campos y resultado | `bool` | Detecta una clave FPS ya existente. |

## Física secuencial y compartida

| Función | Entradas | Salida | Propósito |
|---|---|---|---|
| `updatePhysics` | burbujas por referencia, `deltaTime`, ancho y alto | Estado modificado | Actualiza un cuadro secuencial y limita `deltaTime`. |
| `updatePhysicsSequentialPersistent` | burbujas, `steps: uint64_t`, tiempo y área | Estado modificado | Ejecuta varios pasos reutilizando rejilla y vector de pares; referencia justa del benchmark. |
| `updateSingleBubble` | una burbuja, tiempo y área | Burbuja modificada | Integra posición y refleja velocidad en bordes; kernel independiente por elemento. |
| `calculateCollisionSubsteps` | burbujas y tiempo | `int` | Elige 1–8 subpasos según velocidad y radio para reducir tunneling. |
| `buildCollisionGrid` | burbujas, área, `CollisionGrid&` | Rejilla modificada | Asigna cada burbuja a una celda uniforme. |
| `collectCollisionPairsForBubble` | estado, rejilla, índice, vector de pares | Pares agregados | Busca solapamientos en la celda propia y ocho vecinas. |
| `resolveCollisionPairs` | burbujas, pares, área | Estado modificado | Separa círculos y aplica impulso elástico con masa proporcional al área. |
| `reflectAtBorders` | posición, velocidad, radio y límite | Posición/velocidad modificadas | Reubica dentro del canvas e invierte la componente normal. |
| `collisionPairLess` | dos pares | `bool` | Define el orden determinista por índices. |
| `collisionCellIndex` | rejilla y posición | `int` | Calcula y limita la celda lineal. |
| `resolveCollisionsSequential` | burbujas, área, rejilla y pares | Estado/workspace modificados | Repite detección y resolución hasta diez veces o hasta no tener contactos. |
| `advancePhysicsSequential` | burbujas, tiempo, área y workspace | Estado modificado | Ejecuta todos los subpasos de un paso sin reasignar workspace. |

`CollisionGrid` contiene `cellSize`, filas, columnas, cabezas y enlaces; evita
la búsqueda completa O(N²) en escenarios dispersos. `CollisionPair` guarda
los dos índices de un contacto y se ordena antes de resolverlo.

## OpenMP

| Función | Entradas | Salida | Propósito |
|---|---|---|---|
| `updatePhysicsParallel` | burbujas, tiempo y área | Estado modificado | Adaptador de un paso hacia la región persistente. |
| `updatePhysicsParallelPersistent` | burbujas, pasos, tiempo y área | Estado modificado | Paraleliza movimiento y detección con `omp for`; usa listas locales, `critical`, `barrier` y `single`. |
| `configureOpenMpThreads` | hilos pedidos; procesadores, hilos activos y error por referencia | `bool` | Impide sobresuscripción, desactiva ajuste dinámico y confirma el equipo real. |
| `detectOpenMpThreadCount` | Ninguna | `int` | Consulta cuántos hilos activa el runtime. |

## Renderizador

| Función | Entradas | Salida | Propósito |
|---|---|---|---|
| `render` | `const vector<Bubble>&` | Imagen en framebuffer | Dibuja textura de fondo y círculos translúcidos. |
| `presentRendererFrame` | Ninguna | Ninguna | Intercambia buffers y procesa eventos. |
| `rendererWindowShouldClose` | Ninguna | `bool` | Indica cierre solicitado. |
| `rendererTime` | Ninguna | `double` | Devuelve reloj de GLFW. |
| `setRendererTitle` | `title: const string&` | Ninguna | Muestra variante, N y FPS. |
| `rendererDimensions` | Ninguna | `RendererDimensions` | Obtiene tamaños actuales. |
| `queryWindowMonitorRefreshHz` | Ninguna | `optional<int>` | Consulta refresco del monitor cuando está disponible. |
| `measurementWindowIsStable` | dimensiones esperadas y error | `bool` | Rechaza cierre, minimización o cambio de tamaño durante una medición. |

### Auxiliares privados de visualización y renderizado

| Función | Entradas | Salida | Propósito |
|---|---|---|---|
| `executeVisualFrame` | burbujas y tiempo anterior | Estado y tiempo modificados | Obtiene delta, actualiza física, dibuja y presenta un cuadro. |
| `glfwErrorCallback` | código y descripción | Ninguna | Reporta errores de GLFW. |
| `windowSizeCallback` | ventana, ancho y alto | Estado del renderer | Actualiza dimensiones lógicas con sus mínimos. |
| `framebufferSizeCallback` | ventana, ancho y alto | Estado/OpenGL | Actualiza dimensiones físicas y el viewport. |
| `keyCallback` | ventana, tecla, acción | Estado de ventana | Cierra al presionar Escape. |
| `compileShader` | tipo y fuente GLSL | `GLuint` | Compila un shader y reporta el log de error. |
| `createShaderProgram` | fuentes de vértice y fragmento | `GLuint` | Enlaza un programa y destruye shaders intermedios. |
| `createBubbleShaderProgram` | Ninguna | `GLuint` | Construye el programa para círculos con alpha. |
| `createBackgroundShaderProgram` | Ninguna | `GLuint` | Construye el programa para la textura de fondo. |
| `createCircleGeometry` | Ninguna | `bool` | Crea VAO/VBO del círculo unitario. |
| `createBackgroundGeometry` | Ninguna | `bool` | Crea VAO/VBO del cuadrilátero de pantalla. |
| `loadBackgroundTexture` | ruta de imagen | `bool` | Decodifica JPEG y configura la textura OpenGL. |
| `initializeRendererImpl` | `RendererConfig` | `bool` | Inicializa GLFW, ventana, GLEW, shaders, geometría y textura. |
| `cleanupRendererImpl` | Ninguna | Ninguna | Destruye recursos OpenGL, ventana y GLFW de forma idempotente. |

Los constructores y destructor de `RendererSession`, junto con
`isReady()` y `close()`, envuelven `initializeRendererImpl` y
`cleanupRendererImpl`. Las rutinas de terceros incluidas en
`stb_image.h` no se catalogan porque no son código de autoría del equipo.
