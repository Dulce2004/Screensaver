#include "bubbles/cli.hpp"
#include "bubbles/generation.hpp"
#include "bubbles/physics.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

using bubbles::Bubble;

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

bool parseVisualInvocation() {
    std::vector<std::string> arguments{"BubbleScreensaver.exe", "10", "42"};
    std::vector<char*> argv;
    for (std::string& argument : arguments) {
        argv.push_back(argument.data());
    }

    bubbles::ProgramOptions options;
    std::string error;
    return expect(
               bubbles::parseCommandLine(
                   static_cast<int>(argv.size()), argv.data(), options, error),
               "una invocacion visual valida debe aceptarse")
        && expect(options.mode == bubbles::RunMode::Visual,
                  "la invocacion debe seleccionar el modo visual")
        && expect(options.bubbleCount == 10, "N debe conservarse")
        && expect(options.seed == 42, "la semilla debe conservarse");
}

bool rejectZeroBubbles() {
    std::vector<std::string> arguments{"BubbleScreensaver.exe", "0"};
    std::vector<char*> argv;
    for (std::string& argument : arguments) {
        argv.push_back(argument.data());
    }

    bubbles::ProgramOptions options;
    std::string error;
    return expect(
               !bubbles::parseCommandLine(
                   static_cast<int>(argv.size()), argv.data(), options, error),
               "N=0 debe rechazarse")
        && expect(error == "N debe estar entre 1 y 100000.",
                  "N=0 debe producir el diagnostico defensivo conocido");
}

double maximumPenetration(const std::vector<Bubble>& bubblesToCheck) {
    double maximum = 0.0;
    for (std::size_t first = 0; first < bubblesToCheck.size(); ++first) {
        for (std::size_t second = first + 1;
             second < bubblesToCheck.size();
             ++second) {
            const double dx = bubblesToCheck[second].position.x
                - bubblesToCheck[first].position.x;
            const double dy = bubblesToCheck[second].position.y
                - bubblesToCheck[first].position.y;
            const double distance = std::sqrt(dx * dx + dy * dy);
            maximum = std::max(
                maximum,
                static_cast<double>(bubblesToCheck[first].radius
                                    + bubblesToCheck[second].radius)
                    - distance);
        }
    }
    return std::max(maximum, 0.0);
}

bool preserveGenerationAndPhysics() {
    constexpr float width = 800.0F;
    constexpr float height = 600.0F;
    constexpr double deltaTime = 1.0 / 60.0;

    std::vector<Bubble> generated =
        bubbles::generateBubbles(10, 42, width, height);
    if (!expect(bubbles::checksumBubbleState(generated)
                    == UINT64_C(0x205BA9EAF4A754DC),
                "la modularizacion no debe cambiar la generacion reproducible")) {
        return false;
    }

    bubbles::updatePhysics(generated, deltaTime, width, height);
    if (!expect(bubbles::checksumBubbleState(generated)
                    == UINT64_C(0x6CBE02620FDB33C1),
                "la modularizacion no debe cambiar un paso de fisica")) {
        return false;
    }

    const std::vector<Bubble> dense =
        bubbles::generateBubbles(1000, 42, width, height);
    return expect(maximumPenetration(dense) <= 0.01,
                  "la generacion debe conservar burbujas inicialmente separadas");
}

bool preserveHeadOnCollision() {
    constexpr float width = 800.0F;
    constexpr float height = 600.0F;
    constexpr double deltaTime = 1.0 / 60.0;
    std::vector<Bubble> pair{
        {{300.0F, 300.0F}, {100.0F, 0.0F}, 20.0F, {1, 0, 0, 0.4F}},
        {{500.0F, 300.0F}, {-100.0F, 0.0F}, 20.0F, {0, 1, 0, 0.4F}}
    };

    bool collisionObserved = false;
    for (int step = 0; step < 90; ++step) {
        bubbles::updatePhysics(pair, deltaTime, width, height);
        collisionObserved = collisionObserved
            || (pair[0].velocity.x < 0.0F && pair[1].velocity.x > 0.0F);
    }
    return expect(collisionObserved, "dos burbujas frontales deben rebotar")
        && expect(maximumPenetration(pair) <= 0.01,
                  "el choque frontal no debe dejar penetracion visible");
}

#ifdef BUBBLES_ENABLE_OPENMP
bool preserveOpenMpEquivalence() {
    constexpr float width = 800.0F;
    constexpr float height = 600.0F;
    constexpr double deltaTime = 1.0 / 60.0;

    int availableProcessors = 0;
    int activeThreads = 0;
    std::string error;
    if (!expect(bubbles::configureOpenMpThreads(
                    2, availableProcessors, activeThreads, error),
                "OpenMP debe poder configurar dos hilos")) {
        return false;
    }

    std::vector<Bubble> sequential =
        bubbles::generateBubbles(200, 77, width, height);
    std::vector<Bubble> parallel = sequential;
    for (int step = 0; step < 120; ++step) {
        bubbles::updatePhysics(sequential, deltaTime, width, height);
    }
    bubbles::updatePhysicsParallelPersistent(
        parallel, 120, deltaTime, width, height);

    return expect(activeThreads == 2, "OpenMP debe activar dos hilos")
        && expect(bubbles::checksumBubbleState(sequential)
                      == bubbles::checksumBubbleState(parallel),
                  "la ruta OpenMP debe conservar el estado secuencial");
}
#endif

}  // namespace

int main() {
    bool passed = parseVisualInvocation()
        && rejectZeroBubbles()
        && preserveGenerationAndPhysics()
        && preserveHeadOnCollision();
#ifdef BUBBLES_ENABLE_OPENMP
    passed = preserveOpenMpEquivalence() && passed;
#endif
    if (!passed) {
        return 1;
    }
    std::cout << "core_behavior=PASS\n";
    return 0;
}
