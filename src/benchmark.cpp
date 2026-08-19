#include "bubbles/modes.hpp"

#include "bubbles/csv.hpp"
#include "bubbles/generation.hpp"
#include "bubbles/physics.hpp"
#include "config.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace bubbles {

int runBenchmark(const ProgramOptions& options) {
    // El benchmark no llama init(), no crea ventana y no toca ningun recurso de
    // OpenGL. La generacion queda fuera del intervalo cronometrado: solo se mide
    // la estrategia secuencial o paralela. Ambas conservan su region de trabajo
    // durante todos los pasos para que la comparacion mida el mismo algoritmo.
    const bool useParallelKernel = options.mode == RunMode::BenchmarkParallel;

#ifndef BUBBLES_ENABLE_OPENMP
    if (useParallelKernel) {
        std::cerr << "Error: este ejecutable fue compilado sin soporte OpenMP. "
                     "Use BubbleScreensaverOpenMP.exe.\n";
        return static_cast<int>(ExitCode::UnsupportedMode);
    }
#endif

    int activeThreads = 1;
#ifdef BUBBLES_ENABLE_OPENMP
    int availableProcessors = 1;
    if (useParallelKernel) {
        std::string configurationError;
        if (!configureOpenMpThreads(options.threadCount,
                                    availableProcessors,
                                    activeThreads,
                                    configurationError)) {
            std::cerr << "Error: " << configurationError << '\n';
            return static_cast<int>(ExitCode::InvalidArguments);
        }
    }
#endif

    std::vector<Bubble> bubbles = generateBubbles(
        options.bubbleCount,
        options.seed,
        static_cast<float>(kInitialWindowWidth),
        static_cast<float>(kInitialWindowHeight));

    BenchmarkResult result{};
    result.implementation = useParallelKernel ? "parallel" : "sequential";
    result.threadCount = useParallelKernel ? activeThreads : 1;
    result.bubbleCount = bubbles.size();
    result.seed = options.seed;
    result.steps = options.steps;
    result.repeat = options.repeat;
    result.initialChecksum = checksumBubbleState(bubbles);

    // El checksum final consume todos los campos actualizados. Estas barreras de
    // compilador, junto con las llamadas opacas a steady_clock::now(), impiden
    // mover el kernel fuera de la region medida sin agregar trabajo por step.
    std::atomic_signal_fence(std::memory_order_seq_cst);
    const auto startTime = std::chrono::steady_clock::now();
    std::atomic_signal_fence(std::memory_order_seq_cst);
    if (useParallelKernel) {
#ifdef BUBBLES_ENABLE_OPENMP
        updatePhysicsParallelPersistent(
            bubbles,
            options.steps,
            kBenchmarkDeltaTime,
            static_cast<float>(kInitialWindowWidth),
            static_cast<float>(kInitialWindowHeight));
#endif
    } else {
        updatePhysicsSequentialPersistent(
            bubbles,
            options.steps,
            kBenchmarkDeltaTime,
            static_cast<float>(kInitialWindowWidth),
            static_cast<float>(kInitialWindowHeight));
    }
    std::atomic_signal_fence(std::memory_order_seq_cst);
    const auto endTime = std::chrono::steady_clock::now();
    std::atomic_signal_fence(std::memory_order_seq_cst);

    result.totalMilliseconds =
        std::chrono::duration<double, std::milli>(endTime - startTime).count();
    result.averageStepMilliseconds =
        result.totalMilliseconds / static_cast<double>(options.steps);
    result.nanosecondsPerElementStep =
        result.totalMilliseconds * 1'000'000.0
        / (static_cast<double>(options.bubbleCount)
           * static_cast<double>(options.steps));
    result.finalChecksum = checksumBubbleState(bubbles);

    std::cout << "Mode: "
              << (useParallelKernel ? "benchmark-parallel" : "benchmark") << '\n'
              << "Implementation: " << result.implementation << '\n'
              << "Threads: " << result.threadCount << '\n'
              << "N: " << result.bubbleCount << '\n'
              << "Seed: " << result.seed << '\n'
              << "Steps: " << result.steps << '\n'
              << "Repeat: " << result.repeat << '\n'
              << "Delta time (s): " << std::setprecision(10)
              << kBenchmarkDeltaTime << '\n'
              << "Initial checksum: " << formatChecksum(result.initialChecksum) << '\n'
              << "Final checksum: " << formatChecksum(result.finalChecksum) << '\n'
              << std::fixed << std::setprecision(6)
              << "Total time (ms): " << result.totalMilliseconds << '\n'
              << "Average step time (ms): " << result.averageStepMilliseconds << '\n'
              << "Time per element per step (ns): "
              << result.nanosecondsPerElementStep << '\n';

#ifdef BUBBLES_ENABLE_OPENMP
    if (useParallelKernel) {
        std::cout << "OpenMP strategy: persistent parallel region\n";
        std::cout << "OpenMP processors available: " << availableProcessors << '\n';
    }
#endif

    if (options.csvPath.has_value()) {
        std::string errorMessage;
        if (!appendBenchmarkCsv(*options.csvPath, result, errorMessage)) {
            std::cerr << "Error: " << errorMessage << '\n';
            return static_cast<int>(ExitCode::OutputError);
        }
        std::cout << "CSV: " << *options.csvPath << '\n';
    }

    return static_cast<int>(ExitCode::Success);
}

}  // namespace bubbles
