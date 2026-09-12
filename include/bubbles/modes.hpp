/**
 * @file modes.hpp
 * @brief Puntos de entrada de los modos de ejecución de la aplicación.
 */

#pragma once

#include "bubbles/types.hpp"

namespace bubbles {

/**
 * @brief Ejecuta el benchmark físico secuencial o paralelo.
 * @param options Configuración validada del benchmark.
 * @return Un valor de ExitCode convertido a `int`.
 */
int runBenchmark(const ProgramOptions& options);

/**
 * @brief Ejecuta una medición visual controlada y la persiste en CSV.
 * @param options Configuración validada de ventana, duración y salida.
 * @return Un valor de ExitCode convertido a `int`.
 */
int runFpsSupplementary(const ProgramOptions& options);

/**
 * @brief Ejecuta el screensaver interactivo hasta cerrar la ventana.
 * @param options Cantidad de burbujas y semilla ya validadas.
 * @return Un valor de ExitCode convertido a `int`.
 */
int runVisual(const ProgramOptions& options);

}  // namespace bubbles
