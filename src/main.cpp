#include "bubbles/cli.hpp"
#include "bubbles/modes.hpp"
#include "bubbles/types.hpp"

#include <exception>
#include <iostream>
#include <new>
#include <string>

int main(int argc, char** argv) {
    using namespace bubbles;
    try {
        ProgramOptions options;
        std::string errorMessage;
        if (!parseCommandLine(argc, argv, options, errorMessage)) {
            std::cerr << "Error: " << errorMessage << "\n\n";
            printUsage(std::cerr);
            return static_cast<int>(ExitCode::InvalidArguments);
        }

        if (options.mode == RunMode::Help) {
            printUsage(std::cout);
            return static_cast<int>(ExitCode::Success);
        }
        if (options.mode == RunMode::Benchmark
            || options.mode == RunMode::BenchmarkParallel) {
            return runBenchmark(options);
        }
        if (options.mode == RunMode::FpsSupplementary) {
            return runFpsSupplementary(options);
        }
        return runVisual(options);
    } catch (const std::bad_alloc&) {
        std::cerr << "Error: memoria insuficiente para crear las burbujas solicitadas.\n";
        return static_cast<int>(ExitCode::RuntimeError);
    } catch (const std::exception& exception) {
        std::cerr << "Error no recuperable: " << exception.what() << '\n';
        return static_cast<int>(ExitCode::RuntimeError);
    } catch (...) {
        std::cerr << "Error no recuperable de tipo desconocido.\n";
        return static_cast<int>(ExitCode::RuntimeError);
    }
}
