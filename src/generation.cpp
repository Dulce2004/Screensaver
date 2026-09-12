/**
 * @file generation.cpp
 * @brief Generación pseudoaleatoria reproducible y checksum del estado físico.
 */

#include "bubbles/generation.hpp"

#include "config.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace bubbles {

namespace {

/**
 * @brief Convierte un color HSV a componentes RGB.
 * @param hue Matiz normalizado en el intervalo `[0, 1]`.
 * @param saturation Saturación normalizada.
 * @param value Brillo normalizado.
 * @return Componentes RGB normalizados.
 */
std::array<float, 3> hsvToRgb(float hue, float saturation, float value) {
    const float scaledHue = hue * 6.0F;
    const int sector = static_cast<int>(scaledHue) % 6;
    const float fraction = scaledHue - std::floor(scaledHue);
    const float p = value * (1.0F - saturation);
    const float q = value * (1.0F - fraction * saturation);
    const float t = value * (1.0F - (1.0F - fraction) * saturation);

    switch (sector) {
        case 0: return {value, t, p};
        case 1: return {q, value, p};
        case 2: return {p, value, t};
        case 3: return {p, q, value};
        case 4: return {t, p, value};
        default: return {value, p, q};
    }
}

}  // namespace

std::vector<Bubble> generateBubbles(std::size_t bubbleCount,
                                    std::uint64_t seed,
                                    float areaWidth,
                                    float areaHeight) {
    if (bubbleCount == 0 || bubbleCount > kMaximumBubbleCount) {
        throw std::invalid_argument("Cantidad de burbujas fuera del rango permitido.");
    }
    if (!std::isfinite(areaWidth) || !std::isfinite(areaHeight)
        || areaWidth < 2.0F * kMaximumBubbleRadius
        || areaHeight < 2.0F * kMaximumBubbleRadius) {
        throw std::invalid_argument("Dimensiones invalidas para generar burbujas.");
    }

    std::mt19937_64 generator(seed);
    auto randomUnit = [&generator]() {
        // Se usan 24 bits, la precision efectiva de float. Esta conversion
        // explicita evita depender de uniform_real_distribution y mantiene la
        // secuencia estable con la misma N, seed y ejecutable.
        constexpr float inverseRange = 1.0F / 16'777'216.0F;
        return static_cast<float>(generator() >> 40U) * inverseRange;
    };
    auto randomRange = [&randomUnit](float minimum, float maximum) {
        return minimum + (maximum - minimum) * randomUnit();
    };

    // La rejilla garantiza que las burbujas comiencen separadas. Para valores
    // grandes de N el radio se adapta al espacio disponible en el canvas.
    const float aspectRatio = areaWidth / areaHeight;
    const int columns = std::max(1, static_cast<int>(std::ceil(
        std::sqrt(static_cast<float>(bubbleCount) * aspectRatio))));
    const int rows = std::max(1, static_cast<int>(std::ceil(
        static_cast<float>(bubbleCount) / static_cast<float>(columns))));
    const float cellWidth = areaWidth / static_cast<float>(columns);
    const float cellHeight = areaHeight / static_cast<float>(rows);
    const float maximumGeneratedRadius = std::min(
        kMaximumBubbleRadius, 0.42F * std::min(cellWidth, cellHeight));
    const float minimumGeneratedRadius = std::min(
        kMinimumBubbleRadius, maximumGeneratedRadius * 0.65F);
    const float maximumGeneratedSpeed = std::min(
        kMaximumBubbleSpeed, std::max(12.0F, maximumGeneratedRadius * 12.0F));
    const float minimumGeneratedSpeed = std::min(
        kMinimumBubbleSpeed, maximumGeneratedSpeed * 0.45F);
    const float horizontalJitter = std::max(
        0.0F, (0.5F * cellWidth - maximumGeneratedRadius) * 0.5F);
    const float verticalJitter = std::max(
        0.0F, (0.5F * cellHeight - maximumGeneratedRadius) * 0.5F);

    std::vector<Bubble> bubbles;
    bubbles.reserve(bubbleCount);
    for (std::size_t index = 0; index < bubbleCount; ++index) {
        const int column = static_cast<int>(index % static_cast<std::size_t>(columns));
        const int row = static_cast<int>(index / static_cast<std::size_t>(columns));
        const float radius = randomRange(minimumGeneratedRadius, maximumGeneratedRadius);
        const float direction = randomRange(0.0F, 2.0F * kPi);
        const float speed = randomRange(minimumGeneratedSpeed, maximumGeneratedSpeed);
        const float hue = randomUnit();
        const float saturation = randomRange(0.55F, 0.92F);
        const float brightness = randomRange(0.78F, kMaximumColorComponent);
        const std::array<float, 3> rgb = hsvToRgb(hue, saturation, brightness);

        Bubble bubble{};
        bubble.position = {
            (static_cast<float>(column) + 0.5F) * cellWidth
                + randomRange(-horizontalJitter, horizontalJitter),
            (static_cast<float>(row) + 0.5F) * cellHeight
                + randomRange(-verticalJitter, verticalJitter)
        };
        bubble.velocity = {
            std::cos(direction) * speed,
            std::sin(direction) * speed
        };
        bubble.radius = radius;
        bubble.color = {
            std::max(rgb[0], kMinimumColorComponent),
            std::max(rgb[1], kMinimumColorComponent),
            std::max(rgb[2], kMinimumColorComponent),
            randomRange(kMinimumBubbleAlpha, kMaximumBubbleAlpha)
        };
        bubbles.push_back(bubble);
    }
    return bubbles;
}

namespace {

/**
 * @brief Incorpora bytes de orden fijo a un hash FNV-1a.
 * @param hash Acumulador modificado in situ.
 * @param value Valor entero cuyos bytes menos significativos se consumen.
 * @param byteCount Cantidad de bytes que se agregan.
 */
void appendHashBytes(std::uint64_t& hash, std::uint64_t value, int byteCount) {
    constexpr std::uint64_t fnvPrime = 1'099'511'628'211ULL;
    for (int byteIndex = 0; byteIndex < byteCount; ++byteIndex) {
        hash ^= (value >> (byteIndex * 8)) & 0xFFULL;
        hash *= fnvPrime;
    }
}

}  // namespace

std::uint64_t checksumBubbleState(const std::vector<Bubble>& bubbles) {
    constexpr std::uint64_t fnvOffsetBasis = 14'695'981'039'346'656'037ULL;
    std::uint64_t hash = fnvOffsetBasis;
    appendHashBytes(hash, static_cast<std::uint64_t>(bubbles.size()), 8);

    auto appendFloat = [&hash](float value) {
        static_assert(sizeof(float) == sizeof(std::uint32_t));
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        appendHashBytes(hash, bits, 4);
    };

    for (const Bubble& bubble : bubbles) {
        appendFloat(bubble.position.x);
        appendFloat(bubble.position.y);
        appendFloat(bubble.velocity.x);
        appendFloat(bubble.velocity.y);
        appendFloat(bubble.radius);
        for (const float component : bubble.color) {
            appendFloat(component);
        }
    }
    return hash;
}

std::string formatChecksum(std::uint64_t checksum) {
    std::ostringstream output;
    output << "0x" << std::hex << std::uppercase << std::setw(16)
           << std::setfill('0') << checksum;
    return output.str();
}

}  // namespace bubbles
