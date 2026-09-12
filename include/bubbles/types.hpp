/**
 * @file types.hpp
 * @brief Tipos de datos compartidos por la simulación y sus modos de ejecución.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

/** @brief Componentes principales del screensaver de burbujas. */
namespace bubbles {

/** @brief Vector bidimensional usado para posiciones y velocidades. */
struct Vec2 {
    float x;  ///< Componente horizontal.
    float y;  ///< Componente vertical.
};

/** @brief Estado físico y visual de una burbuja. */
struct Bubble {
    Vec2 position;               ///< Centro en píxeles lógicos del canvas.
    Vec2 velocity;               ///< Velocidad en píxeles por segundo.
    float radius;                ///< Radio en píxeles lógicos.
    std::array<float, 4> color;  ///< Color RGBA con componentes en `[0, 1]`.
};

/** @brief Operación solicitada mediante la línea de comandos. */
enum class RunMode {
    Visual,            ///< Screensaver interactivo.
    Benchmark,         ///< Benchmark secuencial sin renderizado.
    BenchmarkParallel, ///< Benchmark OpenMP sin renderizado.
    FpsSupplementary,  ///< Medición controlada del rendimiento visual.
    Help               ///< Impresión de ayuda y terminación inmediata.
};

/** @brief Códigos de salida estables de la aplicación. */
enum class ExitCode : int {
    Success = 0,          ///< Ejecución completada correctamente.
    RuntimeError = 1,     ///< Fallo de inicialización o de ejecución.
    InvalidArguments = 2, ///< Argumentos ausentes o fuera de rango.
    OutputError = 3,      ///< No fue posible validar o escribir un CSV.
    UnsupportedMode = 4   ///< El binario no admite el modo solicitado.
};

/**
 * @brief Opciones validadas de una ejecución.
 *
 * La CLI construye esta estructura antes de iniciar cualquier modo. Los modos
 * pueden asumir que los límites numéricos y las combinaciones obligatorias ya
 * fueron comprobados.
 */
struct ProgramOptions {
    RunMode mode = RunMode::Visual;       ///< Modo de ejecución seleccionado.
    std::size_t bubbleCount = 0;          ///< Cantidad de burbujas `N`.
    std::uint64_t seed = 0;               ///< Semilla de generación.
    std::uint64_t steps = 0;              ///< Pasos del benchmark físico.
    std::uint64_t repeat = 0;             ///< Identificador de repetición.
    int threadCount = 1;                  ///< Hilos OpenMP solicitados.
    std::optional<std::string> csvPath;   ///< Destino CSV, si corresponde.
    bool seedWasProvided = false;         ///< Indica si la seed vino de la CLI.
    bool vsyncEnabled = true;             ///< Estado solicitado de VSync.
    std::uint64_t warmupSeconds = 5;      ///< Calentamiento visual en segundos.
    std::uint64_t measurementSeconds = 20; ///< Medición visual en segundos.
};

/** @brief Medición primaria producida por el benchmark físico. */
struct BenchmarkResult {
    std::string implementation;          ///< `sequential` o `parallel`.
    int threadCount;                     ///< Hilos realmente utilizados.
    std::size_t bubbleCount;             ///< Cantidad de burbujas procesadas.
    std::uint64_t seed;                  ///< Semilla de la configuración.
    std::uint64_t steps;                 ///< Pasos físicos medidos.
    std::uint64_t repeat;                ///< Repetición de la campaña.
    double totalMilliseconds;            ///< Tiempo total del kernel en ms.
    double averageStepMilliseconds;      ///< Tiempo medio por paso en ms.
    double nanosecondsPerElementStep;    ///< Tiempo normalizado por elemento.
    std::uint64_t initialChecksum;       ///< Hash antes de iniciar el reloj.
    std::uint64_t finalChecksum;         ///< Hash después de la simulación.
};

/** @brief Medición primaria producida por la campaña visual. */
struct FpsResult {
    std::string implementation;                ///< `sequential` u `openmp`.
    int threadCount;                           ///< Hilos realmente utilizados.
    std::size_t bubbleCount;                   ///< Cantidad de burbujas dibujadas.
    std::uint64_t seed;                        ///< Semilla de la configuración.
    std::uint64_t repeat;                      ///< Repetición de la campaña.
    int windowWidth;                           ///< Ancho lógico de ventana.
    int windowHeight;                          ///< Alto lógico de ventana.
    int framebufferWidth;                      ///< Ancho físico del framebuffer.
    int framebufferHeight;                     ///< Alto físico del framebuffer.
    std::optional<int> monitorRefreshHz;       ///< Refresco detectado, en Hz.
    bool vsyncEnabled;                         ///< Estado solicitado de VSync.
    std::uint64_t warmupRequestedSeconds;      ///< Calentamiento solicitado.
    double warmupActualSeconds;                ///< Calentamiento observado.
    std::uint64_t measurementRequestedSeconds; ///< Medición solicitada.
    double measurementActualSeconds;           ///< Medición observada.
    std::uint64_t frames;                      ///< Cuadros presentados.
    double averageFps;                         ///< FPS medios globales.
    double averageFrameMilliseconds;           ///< Tiempo medio de cuadro.
    double minimumIntervalFps;                 ///< Menor FPS en intervalos de 0.5 s.
    double p95FrameMilliseconds;               ///< Percentil 95 de tiempo de cuadro.
    std::uint64_t intervalsBelow60;             ///< Intervalos menores que 60 FPS.
    std::uint64_t intervalCount;                ///< Intervalos FPS contabilizados.
    double intervalsBelow60Percent;             ///< Porcentaje bajo 60 FPS.
    double generationMilliseconds;              ///< Tiempo de generación inicial.
};

}  // namespace bubbles
