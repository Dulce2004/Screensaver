/**
 * @file generation.hpp
 * @brief Generación reproducible y checksums del estado de las burbujas.
 */

#pragma once

#include "bubbles/types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace bubbles {

/**
 * @brief Genera un estado inicial reproducible sin superposiciones.
 * @param bubbleCount Cantidad de burbujas solicitada.
 * @param seed Semilla del generador pseudoaleatorio.
 * @param areaWidth Ancho disponible, en píxeles lógicos.
 * @param areaHeight Alto disponible, en píxeles lógicos.
 * @return Vector con posiciones, velocidades, radios y colores iniciales.
 * @pre @p bubbleCount y las dimensiones deben haber sido validados por la CLI.
 */
std::vector<Bubble> generateBubbles(
    std::size_t bubbleCount,
    std::uint64_t seed,
    float areaWidth,
    float areaHeight);

/**
 * @brief Calcula un hash FNV-1a de todos los campos de las burbujas.
 * @param bubbles Estado que se resume.
 * @return Checksum de 64 bits sensible al contenido y al orden del vector.
 * @note Se usa para comparar las rutas secuencial y OpenMP, no como función
 *       criptográfica.
 */
std::uint64_t checksumBubbleState(const std::vector<Bubble>& bubbles);

/**
 * @brief Formatea un checksum como hexadecimal de 16 dígitos.
 * @param checksum Valor de 64 bits que se representa.
 * @return Texto con prefijo `0x` y ceros a la izquierda.
 */
std::string formatChecksum(std::uint64_t checksum);

}  // namespace bubbles
