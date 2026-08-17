#pragma once

#include <cstddef>
#include <cstdint>

namespace bubbles {

inline constexpr int kInitialWindowWidth = 800;
inline constexpr int kInitialWindowHeight = 600;
inline constexpr int kMinimumWindowWidth = 640;
inline constexpr int kMinimumWindowHeight = 480;
inline constexpr int kCircleSegments = 72;
inline constexpr double kMaximumDeltaTime = 0.05;
inline constexpr double kFpsUpdateInterval = 0.50;
inline constexpr double kBenchmarkDeltaTime = 1.0 / 60.0;
inline constexpr float kPi = 3.14159265358979323846F;
inline constexpr const char* kBackgroundImagePath = "data/raw/Fondo.jpg";

inline constexpr std::size_t kMaximumBubbleCount = 100'000;
inline constexpr std::uint64_t kMaximumBenchmarkSteps = 10'000'000;
inline constexpr std::uint64_t kMaximumBenchmarkElementUpdates = 1'000'000'000;
inline constexpr int kMaximumRequestedThreads = 256;
inline constexpr std::uint64_t kMaximumRepeatIndex = 1'000'000;
inline constexpr std::uint64_t kMaximumFpsPhaseSeconds = 3'600;

inline constexpr const char* kBenchmarkCsvHeader =
    "implementation,threads,N,seed,steps,dt_seconds,repeat,total_ms,"
    "avg_step_ms,ns_per_element_step,initial_checksum,final_checksum";
inline constexpr const char* kFpsCsvHeader =
    "implementation,threads,N,seed,repeat,window_width,window_height,"
    "framebuffer_width,framebuffer_height,monitor_refresh_hz,vsync,"
    "warmup_requested_s,warmup_actual_s,measurement_requested_s,"
    "measurement_actual_s,frames,average_fps,average_frame_ms,"
    "minimum_interval_fps,p95_frame_ms,intervals_below_60,interval_count,"
    "intervals_below_60_percent,generation_ms";

inline constexpr float kMinimumBubbleRadius = 7.0F;
inline constexpr float kMaximumBubbleRadius = 34.0F;
inline constexpr float kMinimumBubbleSpeed = 45.0F;
inline constexpr float kMaximumBubbleSpeed = 185.0F;
inline constexpr float kMinimumColorComponent = 0.18F;
inline constexpr float kMaximumColorComponent = 1.0F;
inline constexpr float kMinimumBubbleAlpha = 0.28F;
inline constexpr float kMaximumBubbleAlpha = 0.48F;
inline constexpr int kCollisionSolverIterations = 10;
inline constexpr int kMaximumCollisionSubsteps = 8;
inline constexpr float kCollisionRestitution = 0.96F;
inline constexpr float kCollisionSeparationEpsilon = 0.001F;

}  // namespace bubbles
