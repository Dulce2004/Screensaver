/**
 * @file csv.hpp
 * @brief Persistencia validada de mediciones físicas y visuales en CSV.
 */

#pragma once

#include "bubbles/types.hpp"

#include <string>

namespace bubbles {

/**
 * @brief Agrega una medición física a un CSV compatible.
 * @param path Ruta del archivo de destino.
 * @param result Medición que se serializa como una fila.
 * @param errorMessage Mensaje de error cuando la operación falla.
 * @retval true La fila y, si hacía falta, el encabezado fueron escritos.
 * @retval false No se modificó un archivo con esquema incompatible o ocurrió
 *         un error de entrada/salida.
 */
bool appendBenchmarkCsv(
    const std::string& path,
    const BenchmarkResult& result,
    std::string& errorMessage);

/**
 * @brief Comprueba que una medición FPS puede agregarse sin duplicados.
 * @param path Ruta del CSV que se desea extender.
 * @param result Configuración cuya identidad se valida.
 * @param writeHeader Recibe `true` si el archivo nuevo necesita encabezado.
 * @param errorMessage Mensaje de error cuando la validación falla.
 * @retval true El esquema es compatible y la configuración no existe.
 * @retval false El archivo no es válido o ya contiene la configuración.
 * @note Esta función no agrega la fila; appendFpsCsv() realiza la escritura.
 */
bool validateFpsCsvAppend(
    const std::string& path,
    const FpsResult& result,
    bool& writeHeader,
    std::string& errorMessage);

/**
 * @brief Agrega una medición FPS y fuerza el vaciado del archivo.
 * @param path Ruta del archivo de destino.
 * @param result Medición visual que se serializa como una fila.
 * @param errorMessage Mensaje de error cuando la operación falla.
 * @retval true La medición quedó persistida correctamente.
 * @retval false Falló la validación, escritura, vaciado o cierre del archivo.
 */
bool appendFpsCsv(
    const std::string& path,
    const FpsResult& result,
    std::string& errorMessage);

}  // namespace bubbles
