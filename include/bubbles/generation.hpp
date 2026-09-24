#pragma once

#include "bubbles/types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace bubbles {

std::vector<Bubble> generateBubbles(
    std::size_t bubbleCount,
    std::uint64_t seed,
    float areaWidth,
    float areaHeight);
std::uint64_t checksumBubbleState(const std::vector<Bubble>& bubbles);
std::string formatChecksum(std::uint64_t checksum);

}  // namespace bubbles

