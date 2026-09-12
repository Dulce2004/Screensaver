/**
 * @file cli.hpp
 * @brief Interfaz de análisis y ayuda de la línea de comandos.
 */

#pragma once

#include "bubbles/types.hpp"

#include <iosfwd>
#include <string>

namespace bubbles {

/**
 * @brief Escribe la sintaxis admitida y los límites de cada argumento.
 * @param output Flujo en el que se imprime la ayuda.
 */
void printUsage(std::ostream& output);

/**
 * @brief Analiza y valida una invocación completa del programa.
 * @param argc Cantidad de argumentos, incluido el nombre del ejecutable.
 * @param argv Arreglo de argumentos terminado según el contrato de `main`.
 * @param options Destino de las opciones cuando la validación tiene éxito.
 * @param errorMessage Descripción legible del primer error encontrado.
 * @retval true La invocación es válida y @p options puede utilizarse.
 * @retval false La invocación es inválida y @p errorMessage explica la causa.
 * @post No crea ventanas ni escribe archivos.
 */
bool parseCommandLine(
    int argc,
    char** argv,
    ProgramOptions& options,
    std::string& errorMessage);

}  // namespace bubbles
