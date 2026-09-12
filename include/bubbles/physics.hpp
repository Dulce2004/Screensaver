/**
 * @file physics.hpp
 * @brief Kernels físicos secuencial y OpenMP de la simulación.
 */

#pragma once

#include "bubbles/types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace bubbles {

/**
 * @brief Avanza un cuadro mediante la implementación secuencial.
 * @param bubbles Estado que se modifica in situ.
 * @param deltaTime Tiempo transcurrido en segundos; se limita internamente.
 * @param areaWidth Ancho del área física, en píxeles lógicos.
 * @param areaHeight Alto del área física, en píxeles lógicos.
 */
void updatePhysics(
    std::vector<Bubble>& bubbles,
    double deltaTime,
    float areaWidth,
    float areaHeight);

/**
 * @brief Ejecuta varios pasos secuenciales reutilizando el workspace.
 * @param bubbles Estado que se modifica in situ.
 * @param steps Cantidad de pasos que se ejecutan.
 * @param deltaTime Duración de cada paso, en segundos.
 * @param areaWidth Ancho del área física, en píxeles lógicos.
 * @param areaHeight Alto del área física, en píxeles lógicos.
 * @note Esta es la referencia justa del benchmark paralelo persistente.
 */
void updatePhysicsSequentialPersistent(
    std::vector<Bubble>& bubbles,
    std::uint64_t steps,
    double deltaTime,
    float areaWidth,
    float areaHeight);

#ifdef BUBBLES_ENABLE_OPENMP
/**
 * @brief Avanza un cuadro mediante la implementación OpenMP.
 * @param bubbles Estado que se modifica in situ.
 * @param deltaTime Tiempo transcurrido en segundos; se limita internamente.
 * @param areaWidth Ancho del área física, en píxeles lógicos.
 * @param areaHeight Alto del área física, en píxeles lógicos.
 */
void updatePhysicsParallel(
    std::vector<Bubble>& bubbles,
    double deltaTime,
    float areaWidth,
    float areaHeight);

/**
 * @brief Ejecuta varios pasos dentro de una región OpenMP persistente.
 * @param bubbles Estado compartido que se modifica in situ.
 * @param steps Cantidad de pasos que se ejecutan.
 * @param deltaTime Duración de cada paso, en segundos.
 * @param areaWidth Ancho del área física, en píxeles lógicos.
 * @param areaHeight Alto del área física, en píxeles lógicos.
 * @pre configureOpenMpThreads() debe haber validado la configuración.
 * @note El movimiento y la detección se distribuyen; la resolución ordenada
 *       permanece en una región `single` para conservar determinismo.
 */
void updatePhysicsParallelPersistent(
    std::vector<Bubble>& bubbles,
    std::uint64_t steps,
    double deltaTime,
    float areaWidth,
    float areaHeight);

/**
 * @brief Valida y fija la cantidad de hilos del runtime OpenMP.
 * @param requestedThreads Cantidad solicitada por la línea de comandos.
 * @param availableProcessors Recibe los procesadores reportados por OpenMP.
 * @param activeThreads Recibe la cantidad observada dentro de la región.
 * @param errorMessage Explica por qué la configuración fue rechazada.
 * @retval true El runtime creó exactamente el equipo solicitado.
 * @retval false La solicitud excede los recursos o el equipo no coincide.
 * @post El ajuste dinámico de hilos queda desactivado cuando tiene éxito.
 */
bool configureOpenMpThreads(
    int requestedThreads,
    int& availableProcessors,
    int& activeThreads,
    std::string& errorMessage);

/**
 * @brief Consulta la cantidad de hilos del equipo OpenMP activo por defecto.
 * @return Cantidad observada dentro de una región paralela.
 */
int detectOpenMpThreadCount();
#endif

}  // namespace bubbles
