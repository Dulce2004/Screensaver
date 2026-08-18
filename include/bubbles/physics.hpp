#pragma once

#include "bubbles/types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace bubbles {

void updatePhysics(
    std::vector<Bubble>& bubbles,
    double deltaTime,
    float areaWidth,
    float areaHeight);
void updatePhysicsSequentialPersistent(
    std::vector<Bubble>& bubbles,
    std::uint64_t steps,
    double deltaTime,
    float areaWidth,
    float areaHeight);

#ifdef BUBBLES_ENABLE_OPENMP
void updatePhysicsParallel(
    std::vector<Bubble>& bubbles,
    double deltaTime,
    float areaWidth,
    float areaHeight);
void updatePhysicsParallelPersistent(
    std::vector<Bubble>& bubbles,
    std::uint64_t steps,
    double deltaTime,
    float areaWidth,
    float areaHeight);
bool configureOpenMpThreads(
    int requestedThreads,
    int& availableProcessors,
    int& activeThreads,
    std::string& errorMessage);
int detectOpenMpThreadCount();
#endif

}  // namespace bubbles
