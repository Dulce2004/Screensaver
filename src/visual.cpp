#include "bubbles/modes.hpp"

#include "bubbles/csv.hpp"
#include "bubbles/generation.hpp"
#include "bubbles/physics.hpp"
#include "bubbles/renderer.hpp"
#include "config.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace bubbles {
namespace {

void executeVisualFrame(std::vector<Bubble>& bubbles, double& previousTime) {
    const double currentTime = rendererTime();
    const double deltaTime = currentTime - previousTime;
    previousTime = currentTime;
    const RendererDimensions dimensions = rendererDimensions();

#ifdef BUBBLES_ENABLE_OPENMP
    updatePhysicsParallel(
        bubbles,
        deltaTime,
        static_cast<float>(dimensions.windowWidth),
        static_cast<float>(dimensions.windowHeight));
#else
    updatePhysics(
        bubbles,
        deltaTime,
        static_cast<float>(dimensions.windowWidth),
        static_cast<float>(dimensions.windowHeight));
#endif
    render(bubbles);
    presentRendererFrame();
}

}  // namespace

int runFpsSupplementary(const ProgramOptions& options) {
    int activeThreads = 1;
#ifdef BUBBLES_ENABLE_OPENMP
    int availableProcessors = 1;
    std::string configurationError;
    if (!configureOpenMpThreads(options.threadCount,
                                availableProcessors,
                                activeThreads,
                                configurationError)) {
        std::cerr << "Error: " << configurationError << '\n';
        return static_cast<int>(ExitCode::InvalidArguments);
    }
#endif

    FpsResult result{};
#ifdef BUBBLES_ENABLE_OPENMP
    result.implementation = "openmp";
#else
    result.implementation = "sequential";
#endif
    result.threadCount = activeThreads;
    result.bubbleCount = options.bubbleCount;
    result.seed = options.seed;
    result.repeat = options.repeat;
    result.vsyncEnabled = options.vsyncEnabled;
    result.warmupRequestedSeconds = options.warmupSeconds;
    result.measurementRequestedSeconds = options.measurementSeconds;

    bool unusedWriteHeader = true;
    std::string csvPreflightError;
    if (!validateFpsCsvAppend(*options.csvPath,
                              result,
                              unusedWriteHeader,
                              csvPreflightError)) {
        std::cerr << "Error: " << csvPreflightError << '\n';
        return static_cast<int>(ExitCode::OutputError);
    }

    const auto generationStart = std::chrono::steady_clock::now();
    std::vector<Bubble> bubbles = generateBubbles(
        options.bubbleCount,
        options.seed,
        static_cast<float>(kInitialWindowWidth),
        static_cast<float>(kInitialWindowHeight));
    const auto generationEnd = std::chrono::steady_clock::now();
    const double generationMilliseconds =
        std::chrono::duration<double, std::milli>(
            generationEnd - generationStart).count();

    std::cout << "Mode: fps-supplementary\n"
              << "Implementation: "
#ifdef BUBBLES_ENABLE_OPENMP
              << "openmp\n"
#else
              << "sequential\n"
#endif
              << "Threads: " << activeThreads << '\n'
              << "N: " << bubbles.size() << '\n'
              << "Seed: " << options.seed << '\n'
              << "Repeat: " << options.repeat << '\n'
              << "VSync requested: "
              << (options.vsyncEnabled ? "on" : "off") << '\n'
              << "Warmup requested (s): " << options.warmupSeconds << '\n'
              << "Measurement requested (s): "
              << options.measurementSeconds << '\n'
              << std::fixed << std::setprecision(6)
              << "Generation time (ms): " << generationMilliseconds << '\n';

    RendererSession rendererSession(
        {bubbles.size(), activeThreads, options.vsyncEnabled, true});
    if (!rendererSession.isReady()) {
        bubbles.clear();
        return static_cast<int>(ExitCode::RuntimeError);
    }

    const RendererDimensions expectedDimensions = rendererDimensions();
    const int windowWidth = expectedDimensions.windowWidth;
    const int windowHeight = expectedDimensions.windowHeight;
    const int framebufferWidth = expectedDimensions.framebufferWidth;
    const int framebufferHeight = expectedDimensions.framebufferHeight;
    const std::optional<int> monitorRefreshHz = queryWindowMonitorRefreshHz();

    std::ostringstream warmupTitle;
    warmupTitle << "FPS suplementario - "
                << visualVariantLabel(activeThreads) << " | N: "
                << bubbles.size() << " | VSync: "
                << (options.vsyncEnabled ? "ON" : "OFF")
                << " | Calentamiento";
    setRendererTitle(warmupTitle.str());

    using SteadyClock = std::chrono::steady_clock;
    double previousTime = rendererTime();
    const auto warmupStart = SteadyClock::now();
    std::string windowError;
    bool validWindow = true;
    while (std::chrono::duration<double>(SteadyClock::now() - warmupStart).count()
               < static_cast<double>(options.warmupSeconds)) {
        if (!measurementWindowIsStable(expectedDimensions, windowError)) {
            validWindow = false;
            break;
        }
        executeVisualFrame(bubbles, previousTime);
    }
    const auto warmupEnd = SteadyClock::now();
    const double warmupActualSeconds =
        std::chrono::duration<double>(warmupEnd - warmupStart).count();
    if (!validWindow) {
        std::cerr << "Error: " << windowError << '\n';
        bubbles.clear();
        return static_cast<int>(ExitCode::RuntimeError);
    }

    std::ostringstream measurementTitle;
    measurementTitle << "FPS suplementario - "
                     << visualVariantLabel(activeThreads) << " | N: "
                     << bubbles.size() << " | VSync: "
                     << (options.vsyncEnabled ? "ON" : "OFF")
                     << " | Midiendo";
    setRendererTitle(measurementTitle.str());

    std::vector<double> frameTimesMilliseconds;
    const std::size_t estimatedFrames = static_cast<std::size_t>(
        std::min<std::uint64_t>(options.measurementSeconds * 240U + 1U,
                                1'000'000U));
    frameTimesMilliseconds.reserve(estimatedFrames);
    std::vector<double> intervalFpsSamples;
    intervalFpsSamples.reserve(static_cast<std::size_t>(
        options.measurementSeconds / kFpsUpdateInterval + 2.0));

    previousTime = rendererTime();
    const auto measurementStart = SteadyClock::now();
    auto intervalStart = measurementStart;
    std::uint64_t frames = 0;
    std::uint64_t framesInInterval = 0;
    while (std::chrono::duration<double>(SteadyClock::now() - measurementStart).count()
               < static_cast<double>(options.measurementSeconds)) {
        if (!measurementWindowIsStable(expectedDimensions, windowError)) {
            validWindow = false;
            break;
        }

        const auto frameStart = SteadyClock::now();
        executeVisualFrame(bubbles, previousTime);
        const auto frameEnd = SteadyClock::now();
        frameTimesMilliseconds.push_back(
            std::chrono::duration<double, std::milli>(frameEnd - frameStart).count());
        ++frames;
        ++framesInInterval;

        const double intervalSeconds =
            std::chrono::duration<double>(frameEnd - intervalStart).count();
        if (intervalSeconds >= kFpsUpdateInterval) {
            intervalFpsSamples.push_back(
                static_cast<double>(framesInInterval) / intervalSeconds);
            framesInInterval = 0;
            intervalStart = frameEnd;
        }
    }
    const auto measurementEnd = SteadyClock::now();
    const double measurementActualSeconds =
        std::chrono::duration<double>(
            measurementEnd - measurementStart).count();

    if (!validWindow) {
        std::cerr << "Error: " << windowError << '\n';
        bubbles.clear();
        return static_cast<int>(ExitCode::RuntimeError);
    }
    if (frames == 0 || frameTimesMilliseconds.empty()
        || intervalFpsSamples.empty() || measurementActualSeconds <= 0.0) {
        std::cerr << "Error: la fase medida no produjo muestras suficientes.\n";
        bubbles.clear();
        return static_cast<int>(ExitCode::RuntimeError);
    }

    std::vector<double> sortedFrameTimes = frameTimesMilliseconds;
    std::sort(sortedFrameTimes.begin(), sortedFrameTimes.end());
    const std::size_t p95Index = static_cast<std::size_t>(
        std::ceil(0.95 * static_cast<double>(sortedFrameTimes.size()))) - 1U;
    const double averageFps =
        static_cast<double>(frames) / measurementActualSeconds;
    const double averageFrameMilliseconds =
        measurementActualSeconds * 1000.0 / static_cast<double>(frames);
    const double minimumIntervalFps = *std::min_element(
        intervalFpsSamples.begin(), intervalFpsSamples.end());
    const std::uint64_t intervalsBelow60 = static_cast<std::uint64_t>(
        std::count_if(intervalFpsSamples.begin(),
                      intervalFpsSamples.end(),
                      [](double fps) { return fps < 60.0; }));

    result.bubbleCount = bubbles.size();
    result.windowWidth = windowWidth;
    result.windowHeight = windowHeight;
    result.framebufferWidth = framebufferWidth;
    result.framebufferHeight = framebufferHeight;
    result.monitorRefreshHz = monitorRefreshHz;
    result.warmupActualSeconds = warmupActualSeconds;
    result.measurementActualSeconds = measurementActualSeconds;
    result.frames = frames;
    result.averageFps = averageFps;
    result.averageFrameMilliseconds = averageFrameMilliseconds;
    result.minimumIntervalFps = minimumIntervalFps;
    result.p95FrameMilliseconds = sortedFrameTimes[p95Index];
    result.intervalsBelow60 = intervalsBelow60;
    result.intervalCount = intervalFpsSamples.size();
    result.intervalsBelow60Percent =
        100.0 * static_cast<double>(intervalsBelow60)
        / static_cast<double>(intervalFpsSamples.size());
    result.generationMilliseconds = generationMilliseconds;

    rendererSession.close();
    bubbles.clear();

    std::string csvError;
    if (!appendFpsCsv(*options.csvPath, result, csvError)) {
        std::cerr << "Error: " << csvError << '\n';
        return static_cast<int>(ExitCode::OutputError);
    }

    std::cout << "Window: " << result.windowWidth << 'x' << result.windowHeight
              << '\n'
              << "Framebuffer: " << result.framebufferWidth << 'x'
              << result.framebufferHeight << '\n'
              << "Monitor refresh (Hz): ";
    if (result.monitorRefreshHz.has_value()) {
        std::cout << *result.monitorRefreshHz;
    } else {
        std::cout << "unavailable";
    }
    std::cout << '\n'
              << std::fixed << std::setprecision(6)
              << "Warmup actual (s): " << result.warmupActualSeconds << '\n'
              << "Measurement actual (s): "
              << result.measurementActualSeconds << '\n'
              << "Frames: " << result.frames << '\n'
              << "Average FPS: " << result.averageFps << '\n'
              << "Average frame time (ms): "
              << result.averageFrameMilliseconds << '\n'
              << "Minimum 0.5 s interval FPS: "
              << result.minimumIntervalFps << '\n'
              << "P95 frame time (ms): "
              << result.p95FrameMilliseconds << '\n'
              << "Intervals below 60 FPS: " << result.intervalsBelow60
              << '/' << result.intervalCount << " ("
              << result.intervalsBelow60Percent << "%)\n"
              << "CSV: " << *options.csvPath << '\n';
    return static_cast<int>(ExitCode::Success);
}

int runVisual(const ProgramOptions& options) {
    int activeThreads = 1;
#ifdef BUBBLES_ENABLE_OPENMP
    activeThreads = detectOpenMpThreadCount();
#endif
    std::vector<Bubble> bubbles = generateBubbles(
        options.bubbleCount,
        options.seed,
        static_cast<float>(kInitialWindowWidth),
        static_cast<float>(kInitialWindowHeight));

    std::cout << "Mode: visual\n"
              << "Implementation: "
              << visualVariantLabel(activeThreads) << '\n'
              << "N: " << bubbles.size() << '\n'
              << "Seed: " << options.seed
              << (options.seedWasProvided ? " (provided)" : " (generated)") << '\n'
              << "Initial checksum: " << formatChecksum(checksumBubbleState(bubbles))
              << std::endl;

    RendererSession rendererSession(
        {bubbles.size(), activeThreads, true, false});
    if (!rendererSession.isReady()) {
        return static_cast<int>(ExitCode::RuntimeError);
    }

    double previousTime = rendererTime();
    double fpsPeriodStart = previousTime;
    unsigned int framesInPeriod = 0;

    while (!rendererWindowShouldClose()) {
        executeVisualFrame(bubbles, previousTime);

        // La lectura posterior al swap incluye el costo completo del cuadro que
        // se suma al contador, evitando el sesgo de la version inicial.
        ++framesInPeriod;
        const double fpsNow = rendererTime();
        const double elapsed = fpsNow - fpsPeriodStart;
        if (elapsed >= kFpsUpdateInterval) {
            const double fps = static_cast<double>(framesInPeriod) / elapsed;
            std::ostringstream title;
            title << "Burbujas OpenGL - "
                  << visualVariantLabel(activeThreads) << " | N: "
                  << bubbles.size() << " | FPS: "
                  << std::fixed << std::setprecision(1) << fps;
            setRendererTitle(title.str());

            framesInPeriod = 0;
            fpsPeriodStart = fpsNow;
        }
    }

    rendererSession.close();
    bubbles.clear();
    return static_cast<int>(ExitCode::Success);
}

}  // namespace bubbles
