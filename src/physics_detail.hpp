/**
 * @file physics_detail.hpp
 * @brief Contratos internos compartidos por la física secuencial y OpenMP.
 */

#pragma once

#include "bubbles/types.hpp"

#include <cstddef>
#include <vector>

namespace bubbles {
/** @brief Implementación compartida que no forma parte de la API de usuario. */
namespace detail {

/** @brief Par ordenado de índices que identifica un contacto potencial. */
struct CollisionPair {
    std::size_t first;  ///< Índice menor del par.
    std::size_t second; ///< Índice mayor del par.
};

/**
 * @brief Rejilla uniforme representada mediante listas enlazadas por índices.
 *
 * `heads[cell]` apunta a la primera burbuja de una celda y `next[index]` a la
 * siguiente burbuja de esa misma lista. El valor `-1` marca el final.
 */
struct CollisionGrid {
    float cellSize = 1.0F; ///< Lado de una celda, en píxeles lógicos.
    int columns = 1;       ///< Cantidad horizontal de celdas.
    int rows = 1;          ///< Cantidad vertical de celdas.
    std::vector<int> heads; ///< Cabeza de la lista de cada celda.
    std::vector<int> next;  ///< Enlace siguiente de cada burbuja.
};

/**
 * @brief Integra una burbuja y refleja su velocidad contra los bordes.
 * @param bubble Estado individual que se modifica.
 * @param deltaTime Duración del subpaso, en segundos.
 * @param areaWidth Ancho del área física.
 * @param areaHeight Alto del área física.
 */
void updateSingleBubble(Bubble& bubble, float deltaTime, float areaWidth, float areaHeight);

/**
 * @brief Compara pares lexicográficamente para obtener un orden determinista.
 * @param left Par situado a la izquierda de la comparación.
 * @param right Par situado a la derecha de la comparación.
 * @return `true` cuando @p left debe aparecer antes que @p right.
 */
bool collisionPairLess(const CollisionPair& left, const CollisionPair& right);

/**
 * @brief Calcula la celda lineal de una posición y la limita a la rejilla.
 * @param grid Rejilla cuya geometría se consulta.
 * @param position Posición en píxeles lógicos.
 * @return Índice lineal válido dentro de `grid.heads`.
 */
int collisionCellIndex(const CollisionGrid& grid, const Vec2& position);

/**
 * @brief Reconstruye la rejilla uniforme para el estado actual.
 * @param bubbles Burbujas que se insertan en las listas de celdas.
 * @param areaWidth Ancho del área física.
 * @param areaHeight Alto del área física.
 * @param grid Workspace que se redimensiona y rellena.
 */
void buildCollisionGrid(
    const std::vector<Bubble>& bubbles,
    float areaWidth,
    float areaHeight,
    CollisionGrid& grid);

/**
 * @brief Agrega los contactos de una burbuja con su celda y ocho vecinas.
 * @param bubbles Estado de solo lectura.
 * @param grid Rejilla construida para el estado actual.
 * @param first Índice de la primera burbuja de cada par.
 * @param pairs Vector al que se agregan los contactos encontrados.
 */
void collectCollisionPairsForBubble(
    const std::vector<Bubble>& bubbles,
    const CollisionGrid& grid,
    std::size_t first,
    std::vector<CollisionPair>& pairs);

/**
 * @brief Separa contactos y aplica impulsos elásticos en orden.
 * @param bubbles Estado compartido que se modifica.
 * @param pairs Pares previamente ordenados por índice.
 * @param areaWidth Ancho del área física.
 * @param areaHeight Alto del área física.
 */
void resolveCollisionPairs(
    std::vector<Bubble>& bubbles,
    const std::vector<CollisionPair>& pairs,
    float areaWidth,
    float areaHeight);

/**
 * @brief Estima los subpasos necesarios para reducir tunneling.
 * @param bubbles Estado usado para encontrar radio mínimo y velocidad máxima.
 * @param deltaTime Duración total del paso, en segundos.
 * @return Cantidad entre 1 y el máximo configurado de subpasos.
 */
int calculateCollisionSubsteps(
    const std::vector<Bubble>& bubbles,
    float deltaTime);

}  // namespace detail
}  // namespace bubbles
