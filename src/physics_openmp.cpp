#include "bubbles/physics.hpp"

#ifdef BUBBLES_ENABLE_OPENMP

#include "config.hpp"
#include "physics_detail.hpp"

#include <omp.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace bubbles {

void updatePhysicsParallel(std::vector<Bubble>& bubbles,
                           double deltaTime,
                           float areaWidth,
                           float areaHeight) {
    updatePhysicsParallelPersistent(bubbles, 1, deltaTime, areaWidth, areaHeight);
}

void updatePhysicsParallelPersistent(std::vector<Bubble>& bubbles,
                                     std::uint64_t steps,
                                     double deltaTime,
                                     float areaWidth,
                                     float areaHeight) {
    if (steps == 0
        || !std::isfinite(deltaTime) || deltaTime <= 0.0
        || !std::isfinite(areaWidth) || !std::isfinite(areaHeight)
        || areaWidth <= 0.0F || areaHeight <= 0.0F) {
        return;
    }

    const float safeDelta = static_cast<float>(std::min(deltaTime, kMaximumDeltaTime));
    const std::int64_t bubbleCount = static_cast<std::int64_t>(bubbles.size());
    const int maximumThreads = std::max(omp_get_max_threads(), 1);
    int currentSubsteps = 1;
    float currentSubstepDelta = safeDelta;
    bool collisionPairsFound = false;
    detail::CollisionGrid grid;
    std::vector<detail::CollisionPair> pairs;
    pairs.reserve(bubbles.size());
    std::vector<std::vector<detail::CollisionPair>> threadPairs(
        static_cast<std::size_t>(maximumThreads));
    for (std::vector<detail::CollisionPair>& localPairs : threadPairs) {
        localPairs.reserve(std::max<std::size_t>(
            16U, bubbles.size() / static_cast<std::size_t>(maximumThreads)));
    }

    // El movimiento y la deteccion de pares son paralelos. La barrera garantiza
    // que la resolucion determinista nunca observe posiciones incompletas.
    #pragma omp parallel default(none) \
        shared(bubbles, grid, pairs, threadPairs, collisionPairsFound, \
               currentSubsteps, currentSubstepDelta) \
        firstprivate(steps, bubbleCount, safeDelta, areaWidth, areaHeight)
    {
        const int threadId = omp_get_thread_num();
        std::vector<detail::CollisionPair>& localPairs =
            threadPairs[static_cast<std::size_t>(threadId)];
        for (std::uint64_t step = 0; step < steps; ++step) {
            #pragma omp single
            {
                currentSubsteps =
                    detail::calculateCollisionSubsteps(bubbles, safeDelta);
                currentSubstepDelta = safeDelta
                    / static_cast<float>(currentSubsteps);
            }

            // Cada hilo conserva una copia para que el paso siguiente no pueda
            // cambiar el límite mientras otro hilo termina el paso actual.
            const int stepSubsteps = currentSubsteps;
            const float stepSubstepDelta = currentSubstepDelta;
            for (int substep = 0; substep < stepSubsteps; ++substep) {
                #pragma omp for schedule(static)
                for (std::int64_t index = 0; index < bubbleCount; ++index) {
                    detail::updateSingleBubble(
                        bubbles[static_cast<std::size_t>(index)],
                        stepSubstepDelta,
                        areaWidth,
                        areaHeight);
                }

                for (int iteration = 0;
                     iteration < kCollisionSolverIterations;
                     ++iteration) {
                    #pragma omp single
                    {
                        detail::buildCollisionGrid(
                            bubbles, areaWidth, areaHeight, grid);
                        pairs.clear();
                    }

                    localPairs.clear();
                    #pragma omp for schedule(static)
                    for (std::int64_t first = 0; first < bubbleCount; ++first) {
                        detail::collectCollisionPairsForBubble(
                            bubbles, grid, static_cast<std::size_t>(first), localPairs);
                    }

                    #pragma omp critical(bubble_collision_pair_merge)
                    {
                        pairs.insert(
                            pairs.end(), localPairs.begin(), localPairs.end());
                    }

                    #pragma omp barrier
                    #pragma omp single
                    {
                        collisionPairsFound = !pairs.empty();
                        if (collisionPairsFound) {
                            std::sort(
                                pairs.begin(), pairs.end(), detail::collisionPairLess);
                            detail::resolveCollisionPairs(
                                bubbles, pairs, areaWidth, areaHeight);
                        }
                    }

                    if (!collisionPairsFound) {
                        break;
                    }
                }
            }
        }
    }
}

bool configureOpenMpThreads(int requestedThreads,
                            int& availableProcessors,
                            int& activeThreads,
                            std::string& errorMessage) {
    availableProcessors = std::max(omp_get_num_procs(), 1);
    const int runtimeThreadLimit = omp_get_thread_limit();
    if (requestedThreads > availableProcessors) {
        std::ostringstream message;
        message << "Se solicitaron " << requestedThreads
                << " hilos, pero OpenMP informa " << availableProcessors
                << " procesadores disponibles. No se habilita sobresuscripcion.";
        errorMessage = message.str();
        return false;
    }
    if (requestedThreads > runtimeThreadLimit) {
        std::ostringstream message;
        message << "La cantidad solicitada supera omp_get_thread_limit()="
                << runtimeThreadLimit << '.';
        errorMessage = message.str();
        return false;
    }

    // Se deshabilita el ajuste dinamico para impedir que el runtime use una
    // cantidad distinta a la solicitada. La region de prueba ocurre fuera del
    // intervalo medido y confirma el numero real del equipo.
    omp_set_dynamic(0);
    omp_set_num_threads(requestedThreads);
    activeThreads = 0;
    #pragma omp parallel default(none) shared(activeThreads) \
        firstprivate(requestedThreads) num_threads(requestedThreads)
    {
        #pragma omp single
        activeThreads = omp_get_num_threads();
    }

    if (activeThreads != requestedThreads) {
        std::ostringstream message;
        message << "OpenMP activo " << activeThreads << " hilos de "
                << requestedThreads << " solicitados.";
        errorMessage = message.str();
        return false;
    }
    return true;
}

int detectOpenMpThreadCount() {
    int activeThreads = 0;
    #pragma omp parallel default(none) shared(activeThreads)
    {
        #pragma omp single
        activeThreads = omp_get_num_threads();
    }
    return std::max(activeThreads, 1);
}

}  // namespace bubbles

#endif
