/**
 * @file csv.cpp
 * @brief Validación de esquemas y escritura durable de mediciones CSV.
 */

#include "bubbles/csv.hpp"

#include "bubbles/generation.hpp"
#include "config.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace bubbles {

bool appendBenchmarkCsv(const std::string& path,
                        const BenchmarkResult& result,
                        std::string& errorMessage) {
    bool writeHeader = true;
    {
        std::ifstream existingFile(path, std::ios::binary);
        if (existingFile) {
            std::string existingHeader;
            std::getline(existingFile, existingHeader);
            if (!existingHeader.empty() && existingHeader.back() == '\r') {
                existingHeader.pop_back();
            }
            writeHeader = existingHeader.empty();
            if (!writeHeader && existingHeader != kBenchmarkCsvHeader) {
                errorMessage =
                    "El CSV existente usa un encabezado incompatible; se conserva "
                    "sin cambios: " + path;
                return false;
            }
        }
    }

    std::ofstream output(path, std::ios::out | std::ios::app);
    if (!output) {
        errorMessage = "No fue posible abrir el archivo CSV: " + path;
        return false;
    }

    if (writeHeader) {
        output << kBenchmarkCsvHeader << '\n';
    }

    output << std::setprecision(17)
           << result.implementation << ','
           << result.threadCount << ','
           << result.bubbleCount << ','
           << result.seed << ','
           << result.steps << ','
           << kBenchmarkDeltaTime << ','
           << result.repeat << ','
           << result.totalMilliseconds << ','
           << result.averageStepMilliseconds << ','
           << result.nanosecondsPerElementStep << ','
           << formatChecksum(result.initialChecksum) << ','
           << formatChecksum(result.finalChecksum) << '\n';

    // Fuerza la entrega del buffer antes de informar exito. Esto permite
    // detectar fallos tardios de escritura (por ejemplo, en una carpeta
    // sincronizada) que el destructor de ofstream no podria comunicar.
    output.flush();
    if (!output) {
        errorMessage = "Ocurrio un error al escribir el archivo CSV: " + path;
        return false;
    }
    output.close();
    if (!output) {
        errorMessage = "Ocurrio un error al cerrar el archivo CSV: " + path;
        return false;
    }
    return true;
}

namespace {

/**
 * @brief Divide una fila del esquema controlado, que no admite comillas.
 * @param row Fila CSV sin salto final.
 * @return Campos separados por comas, incluidos los campos vacíos.
 */
std::vector<std::string> splitSimpleCsvRow(const std::string& row) {
    // El esquema suplementario solo contiene numeros y etiquetas sin comas;
    // por tanto no necesita comillas CSV. Mantener este parser limitado evita
    // fingir soporte general que el escritor no requiere.
    std::vector<std::string> fields;
    std::size_t fieldStart = 0;
    while (fieldStart <= row.size()) {
        const std::size_t comma = row.find(',', fieldStart);
        if (comma == std::string::npos) {
            fields.emplace_back(row.substr(fieldStart));
            break;
        }
        fields.emplace_back(row.substr(fieldStart, comma - fieldStart));
        fieldStart = comma + 1;
    }
    return fields;
}

/**
 * @brief Compara la clave de configuración de una fila FPS.
 * @param fields Campos de una fila existente.
 * @param result Configuración nueva que se desea insertar.
 * @return `true` cuando implementación, hilos, N, seed y repetición coinciden.
 */
bool isSameFpsConfiguration(const std::vector<std::string>& fields,
                            const FpsResult& result) {
    if (fields.size() != 24) {
        return false;
    }
    return fields[0] == result.implementation
        && fields[1] == std::to_string(result.threadCount)
        && fields[2] == std::to_string(result.bubbleCount)
        && fields[3] == std::to_string(result.seed)
        && fields[4] == std::to_string(result.repeat)
        && fields[10] == (result.vsyncEnabled ? "on" : "off")
        && fields[11] == std::to_string(result.warmupRequestedSeconds)
        && fields[13] == std::to_string(result.measurementRequestedSeconds);
}

}  // namespace

bool validateFpsCsvAppend(const std::string& path,
                          const FpsResult& result,
                          bool& writeHeader,
                          std::string& errorMessage) {
    writeHeader = true;
    {
        std::ifstream existingFile(path, std::ios::binary);
        if (existingFile) {
            std::string existingHeader;
            std::getline(existingFile, existingHeader);
            if (!existingHeader.empty() && existingHeader.back() == '\r') {
                existingHeader.pop_back();
            }
            writeHeader = existingHeader.empty();
            if (!writeHeader && existingHeader != kFpsCsvHeader) {
                errorMessage =
                    "El CSV FPS existente usa un encabezado incompatible; se "
                    "conserva sin cambios: " + path;
                return false;
            }

            std::string row;
            while (std::getline(existingFile, row)) {
                if (!row.empty() && row.back() == '\r') {
                    row.pop_back();
                }
                if (row.empty()) {
                    continue;
                }
                const std::vector<std::string> fields = splitSimpleCsvRow(row);
                if (fields.size() != 24) {
                    errorMessage =
                        "El CSV FPS existente contiene una fila incompatible; se "
                        "conserva sin cambios: " + path;
                    return false;
                }
                if (isSameFpsConfiguration(fields, result)) {
                    errorMessage =
                        "Ya existe una fila con la misma implementacion, hilos, N, "
                        "seed, repeat, VSync y duraciones; no se agrego un duplicado.";
                    return false;
                }
            }
        }
    }
    return true;
}

bool appendFpsCsv(const std::string& path,
                  const FpsResult& result,
                  std::string& errorMessage) {
    bool writeHeader = true;
    if (!validateFpsCsvAppend(path, result, writeHeader, errorMessage)) {
        return false;
    }

    std::ofstream output(path, std::ios::out | std::ios::app);
    if (!output) {
        errorMessage = "No fue posible abrir el archivo CSV FPS: " + path;
        return false;
    }
    if (writeHeader) {
        output << kFpsCsvHeader << '\n';
    }

    output << std::setprecision(17)
           << result.implementation << ','
           << result.threadCount << ','
           << result.bubbleCount << ','
           << result.seed << ','
           << result.repeat << ','
           << result.windowWidth << ','
           << result.windowHeight << ','
           << result.framebufferWidth << ','
           << result.framebufferHeight << ',';
    if (result.monitorRefreshHz.has_value()) {
        output << *result.monitorRefreshHz;
    }
    output << ','
           << (result.vsyncEnabled ? "on" : "off") << ','
           << result.warmupRequestedSeconds << ','
           << result.warmupActualSeconds << ','
           << result.measurementRequestedSeconds << ','
           << result.measurementActualSeconds << ','
           << result.frames << ','
           << result.averageFps << ','
           << result.averageFrameMilliseconds << ','
           << result.minimumIntervalFps << ','
           << result.p95FrameMilliseconds << ','
           << result.intervalsBelow60 << ','
           << result.intervalCount << ','
           << result.intervalsBelow60Percent << ','
           << result.generationMilliseconds << '\n';

    output.flush();
    if (!output) {
        errorMessage = "Ocurrio un error al escribir el archivo CSV FPS: " + path;
        return false;
    }
    output.close();
    if (!output) {
        errorMessage = "Ocurrio un error al cerrar el archivo CSV FPS: " + path;
        return false;
    }
    return true;
}

}  // namespace bubbles
