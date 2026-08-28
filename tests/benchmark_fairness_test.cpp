#include "bubbles/generation.hpp"
#include "bubbles/physics.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main() {
    constexpr float width = 800.0F;
    constexpr float height = 600.0F;
    constexpr double deltaTime = 1.0 / 60.0;
    constexpr std::uint64_t steps = 180;

    const std::vector<bubbles::Bubble> initial =
        bubbles::generateBubbles(200, 77, width, height);
    std::vector<bubbles::Bubble> stepByStep = initial;
    std::vector<bubbles::Bubble> persistentSequential = initial;

    for (std::uint64_t step = 0; step < steps; ++step) {
        bubbles::updatePhysics(stepByStep, deltaTime, width, height);
    }
    bubbles::updatePhysicsSequentialPersistent(
        persistentSequential, steps, deltaTime, width, height);

    bool passed = expect(
        bubbles::checksumBubbleState(stepByStep)
            == bubbles::checksumBubbleState(persistentSequential),
        "el workspace persistente secuencial debe conservar la referencia");

    std::vector<bubbles::Bubble> zeroSteps = initial;
    bubbles::updatePhysicsSequentialPersistent(
        zeroSteps, 0, deltaTime, width, height);
    passed = expect(
        bubbles::checksumBubbleState(zeroSteps)
            == bubbles::checksumBubbleState(initial),
        "cero pasos no debe modificar el estado") && passed;

#ifdef BUBBLES_ENABLE_OPENMP
    int availableProcessors = 0;
    int activeThreads = 0;
    std::string errorMessage;
    if (!bubbles::configureOpenMpThreads(
            2, availableProcessors, activeThreads, errorMessage)) {
        std::cerr << "FAIL: " << errorMessage << '\n';
        return 1;
    }

    std::vector<bubbles::Bubble> persistentParallel = initial;
    bubbles::updatePhysicsParallelPersistent(
        persistentParallel, steps, deltaTime, width, height);
    passed = expect(
        bubbles::checksumBubbleState(persistentSequential)
            == bubbles::checksumBubbleState(persistentParallel),
        "los kernels persistentes deben producir el mismo estado") && passed;
#endif

    if (!passed) {
        return 1;
    }
    std::cout << "benchmark_fairness=PASS\n";
    return 0;
}
