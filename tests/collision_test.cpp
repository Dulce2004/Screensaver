#include "bubbles/generation.hpp"
#include "bubbles/physics.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#ifdef BUBBLES_ENABLE_OPENMP
#include <omp.h>
#endif

using namespace bubbles;

double maximumPenetration(const std::vector<Bubble>& bubbles) {
    double maximum = 0.0;
    for (std::size_t first = 0; first < bubbles.size(); ++first) {
        for (std::size_t second = first + 1; second < bubbles.size(); ++second) {
            const double dx = static_cast<double>(bubbles[second].position.x)
                - bubbles[first].position.x;
            const double dy = static_cast<double>(bubbles[second].position.y)
                - bubbles[first].position.y;
            const double distance = std::sqrt(dx * dx + dy * dy);
            maximum = std::max(maximum,
                static_cast<double>(bubbles[first].radius + bubbles[second].radius)
                - distance);
        }
    }
    return std::max(maximum, 0.0);
}

int main() {
    constexpr float width = 800.0F;
    constexpr float height = 600.0F;
    constexpr double deltaTime = 1.0 / 60.0;
    constexpr double tolerance = 0.01;

    const std::vector<Bubble> generated = generateBubbles(1000, 42, width, height);
    const double initialPenetration = maximumPenetration(generated);
    if (initialPenetration > tolerance) {
        std::cerr << "Solapamiento inicial: " << initialPenetration << '\n';
        return 1;
    }

    Bubble left{{300.0F, 300.0F}, {100.0F, 0.0F}, 20.0F, {1, 0, 0, 0.4F}};
    Bubble right{{500.0F, 300.0F}, {-100.0F, 0.0F}, 20.0F, {0, 1, 0, 0.4F}};
    std::vector<Bubble> headOn{left, right};
    bool collisionObserved = false;
    for (int step = 0; step < 90; ++step) {
        updatePhysics(headOn, deltaTime, width, height);
        if (maximumPenetration(headOn) > tolerance) {
            std::cerr << "Solapamiento tras choque frontal.\n";
            return 2;
        }
        collisionObserved = collisionObserved
            || (headOn[0].velocity.x < 0.0F && headOn[1].velocity.x > 0.0F);
    }
    if (!collisionObserved) {
        std::cerr << "No se observo el rebote entre burbujas.\n";
        return 3;
    }

#ifdef BUBBLES_ENABLE_OPENMP
    omp_set_dynamic(0);
    omp_set_num_threads(4);
    std::vector<Bubble> sequential = generateBubbles(200, 77, width, height);
    std::vector<Bubble> parallel = sequential;
    double greatestPenetration = 0.0;
    for (int step = 0; step < 180; ++step) {
        updatePhysics(sequential, deltaTime, width, height);
        updatePhysicsParallel(parallel, deltaTime, width, height);
        greatestPenetration = std::max(
            greatestPenetration, maximumPenetration(parallel));
        if (checksumBubbleState(sequential) != checksumBubbleState(parallel)) {
            std::cerr << "Diferencia secuencial/OpenMP en el paso " << step << ".\n";
            return 4;
        }
    }
    if (greatestPenetration > tolerance) {
        std::cerr << "Penetracion OpenMP maxima: " << greatestPenetration << '\n';
        return 5;
    }

    std::vector<Bubble> persistentSequential = generateBubbles(200, 77, width, height);
    std::vector<Bubble> persistentParallel = persistentSequential;
    for (int step = 0; step < 180; ++step) {
        updatePhysics(persistentSequential, deltaTime, width, height);
    }
    updatePhysicsParallelPersistent(
        persistentParallel, 180, deltaTime, width, height);
    if (checksumBubbleState(persistentSequential)
        != checksumBubbleState(persistentParallel)) {
        double maximumDifference = 0.0;
        for (std::size_t index = 0; index < persistentSequential.size(); ++index) {
            maximumDifference = std::max(maximumDifference, std::abs(
                static_cast<double>(persistentSequential[index].position.x)
                - persistentParallel[index].position.x));
            maximumDifference = std::max(maximumDifference, std::abs(
                static_cast<double>(persistentSequential[index].position.y)
                - persistentParallel[index].position.y));
            maximumDifference = std::max(maximumDifference, std::abs(
                static_cast<double>(persistentSequential[index].velocity.x)
                - persistentParallel[index].velocity.x));
            maximumDifference = std::max(maximumDifference, std::abs(
                static_cast<double>(persistentSequential[index].velocity.y)
                - persistentParallel[index].velocity.y));
        }
        std::cerr << "Diferencia persistente maxima: " << maximumDifference << '\n';
        return 6;
    }
    std::cout << "initial_penetration=" << initialPenetration << '\n'
              << "head_on_collision=PASS\n"
              << "sequential_openmp_equivalence=PASS\n"
              << "persistent_equivalence=PASS\n"
              << "maximum_parallel_penetration=" << greatestPenetration << '\n';
#endif
    return 0;
}
