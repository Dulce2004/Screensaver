#include "bubbles/cli.hpp"

#include "config.hpp"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>

namespace bubbles {

void printUsage(std::ostream& output) {
    output << "Usage:\n"
           << "  BubbleScreensaver.exe <N> [seed]\n"
           << "  BubbleScreensaver.exe --benchmark <N> <seed> <steps> "
              "[--csv <file>] [--repeat <index>]\n"
           << "  BubbleScreensaverOpenMP.exe --benchmark-parallel <N> <seed> "
              "<steps> <threads> [--csv <file>] [--repeat <index>]\n"
           << "  BubbleScreensaver[OpenMP].exe --fps-supplementary <N> <seed> "
              "<threads> <vsync:on|off> <warmup_s> <measurement_s> "
              "--csv <file> --repeat <index>\n"
           << "  BubbleScreensaver.exe --help\n\n"
           << "N debe estar entre 1 y " << kMaximumBubbleCount << ".\n"
           << "steps debe estar entre 1 y " << kMaximumBenchmarkSteps << ".\n"
           << "threads debe estar entre 1 y " << kMaximumRequestedThreads
           << " y no superar los procesadores disponibles.\n"
           << "repeat debe estar entre 1 y " << kMaximumRepeatIndex
           << " cuando se especifica.\n"
           << "warmup_s puede ser 0.." << kMaximumFpsPhaseSeconds
           << " y measurement_s debe ser 1.." << kMaximumFpsPhaseSeconds << ".\n";
}

namespace {

bool parseUnsigned64(std::string_view text, std::uint64_t& value) {
    if (text.empty()) {
        return false;
    }

    value = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value, 10);
    return result.ec == std::errc{} && result.ptr == end;
}

bool parseBubbleCount(std::string_view text,
                      std::size_t& bubbleCount,
                      std::string& errorMessage) {
    std::uint64_t parsedValue = 0;
    if (!parseUnsigned64(text, parsedValue)) {
        errorMessage = "N debe ser un entero decimal positivo sin signo.";
        return false;
    }
    if (parsedValue == 0 || parsedValue > kMaximumBubbleCount) {
        std::ostringstream message;
        message << "N debe estar entre 1 y " << kMaximumBubbleCount << '.';
        errorMessage = message.str();
        return false;
    }

    bubbleCount = static_cast<std::size_t>(parsedValue);
    return true;
}

bool parseSeed(std::string_view text,
               std::uint64_t& seed,
               std::string& errorMessage) {
    if (!parseUnsigned64(text, seed)) {
        errorMessage = "seed debe ser un entero decimal entre 0 y 18446744073709551615.";
        return false;
    }
    return true;
}

bool parseSteps(std::string_view text,
                std::uint64_t& steps,
                std::string& errorMessage) {
    if (!parseUnsigned64(text, steps)) {
        errorMessage = "steps debe ser un entero decimal positivo sin signo.";
        return false;
    }
    if (steps == 0 || steps > kMaximumBenchmarkSteps) {
        std::ostringstream message;
        message << "steps debe estar entre 1 y " << kMaximumBenchmarkSteps << '.';
        errorMessage = message.str();
        return false;
    }
    return true;
}

bool parseThreadCount(std::string_view text,
                      int& threadCount,
                      std::string& errorMessage) {
    std::uint64_t parsedValue = 0;
    if (!parseUnsigned64(text, parsedValue)) {
        errorMessage = "threads debe ser un entero decimal positivo sin signo.";
        return false;
    }
    if (parsedValue == 0
        || parsedValue > static_cast<std::uint64_t>(kMaximumRequestedThreads)) {
        std::ostringstream message;
        message << "threads debe estar entre 1 y " << kMaximumRequestedThreads << '.';
        errorMessage = message.str();
        return false;
    }
    threadCount = static_cast<int>(parsedValue);
    return true;
}

bool parseRepeat(std::string_view text,
                 std::uint64_t& repeat,
                 std::string& errorMessage) {
    if (!parseUnsigned64(text, repeat)) {
        errorMessage = "repeat debe ser un entero decimal positivo sin signo.";
        return false;
    }
    if (repeat == 0 || repeat > kMaximumRepeatIndex) {
        std::ostringstream message;
        message << "repeat debe estar entre 1 y " << kMaximumRepeatIndex << '.';
        errorMessage = message.str();
        return false;
    }
    return true;
}

bool parseFpsPhaseSeconds(std::string_view text,
                          bool allowZero,
                          const char* fieldName,
                          std::uint64_t& seconds,
                          std::string& errorMessage) {
    if (!parseUnsigned64(text, seconds)) {
        errorMessage = std::string(fieldName)
            + " debe ser un entero decimal sin signo.";
        return false;
    }
    if ((!allowZero && seconds == 0) || seconds > kMaximumFpsPhaseSeconds) {
        std::ostringstream message;
        message << fieldName << " debe estar entre " << (allowZero ? 0 : 1)
                << " y " << kMaximumFpsPhaseSeconds << " segundos.";
        errorMessage = message.str();
        return false;
    }
    return true;
}

bool parseVSync(std::string_view text,
                bool& enabled,
                std::string& errorMessage) {
    if (text == "on") {
        enabled = true;
        return true;
    }
    if (text == "off") {
        enabled = false;
        return true;
    }
    errorMessage = "vsync debe ser exactamente 'on' u 'off'.";
    return false;
}

std::uint64_t createDefaultSeed() {
    // random_device aporta entropia del sistema cuando esta disponible. El reloj
    // evita repetir la semilla en implementaciones donde random_device sea fijo.
    std::random_device randomDevice;
    const auto clockValue = static_cast<std::uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uint64_t seed = clockValue;
    seed ^= static_cast<std::uint64_t>(randomDevice()) << 32U;
    seed ^= static_cast<std::uint64_t>(randomDevice());
    return seed;
}

}  // namespace

bool parseCommandLine(int argc,
                      char** argv,
                      ProgramOptions& options,
                      std::string& errorMessage) {
    if (argc < 1 || argv == nullptr) {
        errorMessage = "No fue posible leer la linea de comandos.";
        return false;
    }
    if (argc == 1) {
        errorMessage = "Falta el parametro obligatorio N.";
        return false;
    }

    const std::string_view firstArgument = argv[1] != nullptr ? argv[1] : "";
    if (firstArgument == "--help") {
        if (argc != 2) {
            errorMessage = "--help no acepta argumentos adicionales.";
            return false;
        }
        options.mode = RunMode::Help;
        return true;
    }

    const bool isSequentialBenchmark = firstArgument == "--benchmark";
    const bool isParallelBenchmark = firstArgument == "--benchmark-parallel";
    if (isSequentialBenchmark || isParallelBenchmark) {
        const int baseArgumentCount = isParallelBenchmark ? 6 : 5;
        const int optionalArgumentCount = argc - baseArgumentCount;
        if (optionalArgumentCount < 0
            || optionalArgumentCount > 4
            || optionalArgumentCount % 2 != 0) {
            errorMessage = isParallelBenchmark
                ? "El benchmark paralelo requiere N, seed, steps y threads; "
                  "--csv <file> y --repeat <index> son opcionales."
                : "El benchmark requiere N, seed y steps; --csv <file> y "
                  "--repeat <index> son opcionales.";
            return false;
        }

        options.mode = isParallelBenchmark
            ? RunMode::BenchmarkParallel
            : RunMode::Benchmark;
        options.seedWasProvided = true;
        if (!parseBubbleCount(argv[2] != nullptr ? argv[2] : "",
                              options.bubbleCount,
                              errorMessage)
            || !parseSeed(argv[3] != nullptr ? argv[3] : "",
                          options.seed,
                          errorMessage)
            || !parseSteps(argv[4] != nullptr ? argv[4] : "",
                           options.steps,
                           errorMessage)) {
            return false;
        }

        if (isParallelBenchmark
            && !parseThreadCount(argv[5] != nullptr ? argv[5] : "",
                                 options.threadCount,
                                 errorMessage)) {
            return false;
        }

        if (options.steps
            > kMaximumBenchmarkElementUpdates
                  / static_cast<std::uint64_t>(options.bubbleCount)) {
            std::ostringstream message;
            message << "N * steps no puede exceder "
                    << kMaximumBenchmarkElementUpdates
                    << " actualizaciones por benchmark.";
            errorMessage = message.str();
            return false;
        }

        bool csvWasProvided = false;
        bool repeatWasProvided = false;
        for (int optionIndex = baseArgumentCount;
             optionIndex < argc;
             optionIndex += 2) {
            const std::string_view option =
                argv[optionIndex] != nullptr ? argv[optionIndex] : "";
            const std::string_view value =
                argv[optionIndex + 1] != nullptr ? argv[optionIndex + 1] : "";

            if (option == "--csv") {
                if (csvWasProvided) {
                    errorMessage = "--csv no puede especificarse mas de una vez.";
                    return false;
                }
                if (value.empty()) {
                    errorMessage = "La ruta de salida CSV no puede estar vacia.";
                    return false;
                }
                options.csvPath = std::string(value);
                csvWasProvided = true;
            } else if (option == "--repeat") {
                if (repeatWasProvided) {
                    errorMessage = "--repeat no puede especificarse mas de una vez.";
                    return false;
                }
                if (!parseRepeat(value, options.repeat, errorMessage)) {
                    return false;
                }
                repeatWasProvided = true;
            } else {
                errorMessage = "Opcion de benchmark desconocida: " + std::string(option);
                return false;
            }
        }
        return true;
    }

    if (firstArgument == "--fps-supplementary") {
        // Todos los controles se hacen obligatorios para que una fila no dependa
        // de valores implicitos ni pueda quedar sin identidad de repeticion.
        if (argc != 12) {
            errorMessage =
                "--fps-supplementary requiere N, seed, threads, vsync, "
                "warmup_s, measurement_s, --csv <file> y --repeat <index>.";
            return false;
        }

        options.mode = RunMode::FpsSupplementary;
        options.seedWasProvided = true;
        if (!parseBubbleCount(argv[2] != nullptr ? argv[2] : "",
                              options.bubbleCount,
                              errorMessage)
            || !parseSeed(argv[3] != nullptr ? argv[3] : "",
                          options.seed,
                          errorMessage)
            || !parseThreadCount(argv[4] != nullptr ? argv[4] : "",
                                 options.threadCount,
                                 errorMessage)
            || !parseVSync(argv[5] != nullptr ? argv[5] : "",
                           options.vsyncEnabled,
                           errorMessage)
            || !parseFpsPhaseSeconds(argv[6] != nullptr ? argv[6] : "",
                                     true,
                                     "warmup_s",
                                     options.warmupSeconds,
                                     errorMessage)
            || !parseFpsPhaseSeconds(argv[7] != nullptr ? argv[7] : "",
                                     false,
                                     "measurement_s",
                                     options.measurementSeconds,
                                     errorMessage)) {
            return false;
        }

#ifndef BUBBLES_ENABLE_OPENMP
        if (options.threadCount != 1) {
            errorMessage =
                "El ejecutable secuencial requiere threads=1 en la prueba FPS.";
            return false;
        }
#endif

        bool csvWasProvided = false;
        bool repeatWasProvided = false;
        for (int optionIndex = 8; optionIndex < argc; optionIndex += 2) {
            const std::string_view option =
                argv[optionIndex] != nullptr ? argv[optionIndex] : "";
            const std::string_view value =
                argv[optionIndex + 1] != nullptr ? argv[optionIndex + 1] : "";
            if (option == "--csv") {
                if (csvWasProvided || value.empty()) {
                    errorMessage = csvWasProvided
                        ? "--csv no puede especificarse mas de una vez."
                        : "La ruta de salida CSV no puede estar vacia.";
                    return false;
                }
                options.csvPath = std::string(value);
                csvWasProvided = true;
            } else if (option == "--repeat") {
                if (repeatWasProvided) {
                    errorMessage = "--repeat no puede especificarse mas de una vez.";
                    return false;
                }
                if (!parseRepeat(value, options.repeat, errorMessage)) {
                    return false;
                }
                repeatWasProvided = true;
            } else {
                errorMessage =
                    "Opcion de prueba FPS desconocida: " + std::string(option);
                return false;
            }
        }
        if (!csvWasProvided || !repeatWasProvided) {
            errorMessage =
                "La prueba FPS requiere tanto --csv como --repeat.";
            return false;
        }
        return true;
    }

    if (firstArgument.size() >= 2 && firstArgument.substr(0, 2) == "--") {
        errorMessage = "Opcion desconocida: " + std::string(firstArgument);
        return false;
    }
    if (argc != 2 && argc != 3) {
        errorMessage = "El modo visual acepta unicamente N y una seed opcional.";
        return false;
    }

    options.mode = RunMode::Visual;
    if (!parseBubbleCount(firstArgument, options.bubbleCount, errorMessage)) {
        return false;
    }

    if (argc == 3) {
        options.seedWasProvided = true;
        if (!parseSeed(argv[2] != nullptr ? argv[2] : "",
                       options.seed,
                       errorMessage)) {
            return false;
        }
    } else {
        options.seed = createDefaultSeed();
    }
    return true;
}

}  // namespace bubbles
