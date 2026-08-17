#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace bubbles {

struct Vec2 {
    float x;
    float y;
};

struct Bubble {
    Vec2 position;
    Vec2 velocity;
    float radius;
    std::array<float, 4> color;
};

enum class RunMode {
    Visual,
    Benchmark,
    BenchmarkParallel,
    FpsSupplementary,
    Help
};

enum class ExitCode : int {
    Success = 0,
    RuntimeError = 1,
    InvalidArguments = 2,
    OutputError = 3,
    UnsupportedMode = 4
};

struct ProgramOptions {
    RunMode mode = RunMode::Visual;
    std::size_t bubbleCount = 0;
    std::uint64_t seed = 0;
    std::uint64_t steps = 0;
    std::uint64_t repeat = 0;
    int threadCount = 1;
    std::optional<std::string> csvPath;
    bool seedWasProvided = false;
    bool vsyncEnabled = true;
    std::uint64_t warmupSeconds = 5;
    std::uint64_t measurementSeconds = 20;
};

struct BenchmarkResult {
    std::string implementation;
    int threadCount;
    std::size_t bubbleCount;
    std::uint64_t seed;
    std::uint64_t steps;
    std::uint64_t repeat;
    double totalMilliseconds;
    double averageStepMilliseconds;
    double nanosecondsPerElementStep;
    std::uint64_t initialChecksum;
    std::uint64_t finalChecksum;
};

struct FpsResult {
    std::string implementation;
    int threadCount;
    std::size_t bubbleCount;
    std::uint64_t seed;
    std::uint64_t repeat;
    int windowWidth;
    int windowHeight;
    int framebufferWidth;
    int framebufferHeight;
    std::optional<int> monitorRefreshHz;
    bool vsyncEnabled;
    std::uint64_t warmupRequestedSeconds;
    double warmupActualSeconds;
    std::uint64_t measurementRequestedSeconds;
    double measurementActualSeconds;
    std::uint64_t frames;
    double averageFps;
    double averageFrameMilliseconds;
    double minimumIntervalFps;
    double p95FrameMilliseconds;
    std::uint64_t intervalsBelow60;
    std::uint64_t intervalCount;
    double intervalsBelow60Percent;
    double generationMilliseconds;
};

}  // namespace bubbles

