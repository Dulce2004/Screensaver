/**
 * @file main.cpp
 * @brief Punto de entrada, despacho de modos y frontera global de excepciones.
 */

#include "bubbles/cli.hpp"
#include "bubbles/modes.hpp"
#include "bubbles/types.hpp"

#include <exception>
#include <iostream>
#include <new>
#include <string>

/**
 * @brief Analiza la CLI y despacha el modo solicitado.
 * @param argc Cantidad de argumentos del proceso.
 * @param argv Valores de la línea de comandos.
 * @return Un valor de bubbles::ExitCode convertido a `int`.
 */
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
