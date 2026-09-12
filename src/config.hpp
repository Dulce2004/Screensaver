/**
 * @file config.hpp
 * @brief Constantes compartidas de validación, simulación y persistencia.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace bubbles {

/** @name Ventana y temporización */
///@{
inline constexpr int kInitialWindowWidth = 800;  ///< Ancho inicial en píxeles.
inline constexpr int kInitialWindowHeight = 600; ///< Alto inicial en píxeles.
inline constexpr int kMinimumWindowWidth = 640;  ///< Ancho mínimo permitido.
inline constexpr int kMinimumWindowHeight = 480; ///< Alto mínimo permitido.
inline constexpr int kCircleSegments = 72;       ///< Segmentos del círculo OpenGL.
inline constexpr double kMaximumDeltaTime = 0.05;///< Paso visual máximo en s.
inline constexpr double kFpsUpdateInterval = 0.50; ///< Intervalo de FPS en s.
inline constexpr double kBenchmarkDeltaTime = 1.0 / 60.0; ///< Paso canónico.
inline constexpr float kPi = 3.14159265358979323846F; ///< Aproximación de pi.
inline constexpr const char* kBackgroundImagePath =
    "data/raw/Fondo.jpg"; ///< Textura cargada por el renderizador.
///@}

/** @name Límites de entrada */
///@{
inline constexpr std::size_t kMaximumBubbleCount = 100'000; ///< Máximo de N.
inline constexpr std::uint64_t kMaximumBenchmarkSteps =
    10'000'000; ///< Máximo de pasos por ejecución.
inline constexpr std::uint64_t kMaximumBenchmarkElementUpdates =
    1'000'000'000; ///< Límite de `N * steps`.
inline constexpr int kMaximumRequestedThreads = 256; ///< Máximo de hilos.
inline constexpr std::uint64_t kMaximumRepeatIndex =
    1'000'000; ///< Mayor identificador de repetición.
inline constexpr std::uint64_t kMaximumFpsPhaseSeconds =
    3'600; ///< Duración máxima de una fase visual.
///@}

/** @name Esquemas CSV canónicos */
///@{
inline constexpr const char* kBenchmarkCsvHeader =
    "implementation,threads,N,seed,steps,dt_seconds,repeat,total_ms,"
    "avg_step_ms,ns_per_element_step,initial_checksum,final_checksum"; ///< Esquema físico.
inline constexpr const char* kFpsCsvHeader =
    "implementation,threads,N,seed,repeat,window_width,window_height,"
    "framebuffer_width,framebuffer_height,monitor_refresh_hz,vsync,"
    "warmup_requested_s,warmup_actual_s,measurement_requested_s,"
    "measurement_actual_s,frames,average_fps,average_frame_ms,"
    "minimum_interval_fps,p95_frame_ms,intervals_below_60,interval_count,"
    "intervals_below_60_percent,generation_ms"; ///< Esquema visual.
///@}

/** @name Parámetros físicos y visuales */
///@{
inline constexpr float kMinimumBubbleRadius = 7.0F; ///< Radio mínimo en píxeles.
inline constexpr float kMaximumBubbleRadius = 34.0F; ///< Radio máximo en píxeles.
inline constexpr float kMinimumBubbleSpeed = 45.0F; ///< Velocidad mínima en px/s.
inline constexpr float kMaximumBubbleSpeed = 185.0F; ///< Velocidad máxima en px/s.
inline constexpr float kMinimumColorComponent = 0.18F; ///< Canal RGB mínimo.
inline constexpr float kMaximumColorComponent = 1.0F; ///< Canal RGB máximo.
inline constexpr float kMinimumBubbleAlpha = 0.28F; ///< Alpha mínimo.
inline constexpr float kMaximumBubbleAlpha = 0.48F; ///< Alpha máximo.
inline constexpr int kCollisionSolverIterations = 10; ///< Iteraciones del solver.
inline constexpr int kMaximumCollisionSubsteps = 8; ///< Subpasos máximos.
inline constexpr float kCollisionRestitution = 0.96F; ///< Coeficiente elástico.
inline constexpr float kCollisionSeparationEpsilon = 0.001F; ///< Separación extra.
///@}

}  // namespace bubbles
