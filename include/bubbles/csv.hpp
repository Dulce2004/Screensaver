#pragma once

#include "bubbles/types.hpp"

#include <string>

namespace bubbles {

bool appendBenchmarkCsv(
    const std::string& path,
    const BenchmarkResult& result,
    std::string& errorMessage);
bool validateFpsCsvAppend(
    const std::string& path,
    const FpsResult& result,
    bool& writeHeader,
    std::string& errorMessage);
bool appendFpsCsv(
    const std::string& path,
    const FpsResult& result,
    std::string& errorMessage);

}  // namespace bubbles

