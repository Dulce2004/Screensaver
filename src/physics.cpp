#include "bubbles/physics.hpp"

#include "config.hpp"
#include "physics_detail.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace bubbles {
namespace detail {

void reflectAtBorders(float& position, float& velocity, float radius, float limit) {
    const float minimum = radius;
    const float maximum = limit - radius;

    // Proteccion ante un canvas accidentalmente menor que el diametro.
    if (maximum < minimum) {
        position = limit * 0.5F;
        velocity = 0.0F;
        return;
    }

    if (position < minimum) {
        position = minimum;
        velocity = std::abs(velocity);
    } else if (position > maximum) {
        position = maximum;
        velocity = -std::abs(velocity);
    }
}

void updateSingleBubble(Bubble& bubble,
                        float deltaTime,
                        float areaWidth,
                        float areaHeight) {
    // Este es el kernel por elemento: solo depende del estado de esta burbuja y
    // de parametros de solo lectura. No consulta ni modifica otras burbujas.
    bubble.position.x += bubble.velocity.x * deltaTime;
    bubble.position.y += bubble.velocity.y * deltaTime;

    reflectAtBorders(bubble.position.x, bubble.velocity.x, bubble.radius, areaWidth);
    reflectAtBorders(bubble.position.y, bubble.velocity.y, bubble.radius, areaHeight);
}

bool collisionPairLess(const CollisionPair& left, const CollisionPair& right) {
    return left.first < right.first
        || (left.first == right.first && left.second < right.second);
}

int collisionCellIndex(const CollisionGrid& grid, const Vec2& position) {
    const int column = std::clamp(
        static_cast<int>(position.x / grid.cellSize), 0, grid.columns - 1);
    const int row = std::clamp(
        static_cast<int>(position.y / grid.cellSize), 0, grid.rows - 1);
    return row * grid.columns + column;
}

// Construye una lista enlazada de burbujas por celda.
void buildCollisionGrid(const std::vector<Bubble>& bubbles,
                        float areaWidth,
                        float areaHeight,
                        CollisionGrid& grid) {
    float maximumRadius = 0.5F;
    for (const Bubble& bubble : bubbles) {
        maximumRadius = std::max(maximumRadius, bubble.radius);
    }

    grid.cellSize = std::max(2.0F * maximumRadius, 1.0F);
    grid.columns = std::max(1, static_cast<int>(std::ceil(areaWidth / grid.cellSize)));
    grid.rows = std::max(1, static_cast<int>(std::ceil(areaHeight / grid.cellSize)));
    const std::size_t cellCount = static_cast<std::size_t>(grid.columns)
        * static_cast<std::size_t>(grid.rows);
    grid.heads.assign(cellCount, -1);
    grid.next.assign(bubbles.size(), -1);

    for (std::size_t index = 0; index < bubbles.size(); ++index) {
        const int cell = collisionCellIndex(grid, bubbles[index].position);
        grid.next[index] = grid.heads[static_cast<std::size_t>(cell)];
        grid.heads[static_cast<std::size_t>(cell)] = static_cast<int>(index);
    }
}

// Busca contactos solo en la celda actual y sus ocho vecinas.
void collectCollisionPairsForBubble(const std::vector<Bubble>& bubbles,
                                    const CollisionGrid& grid,
                                    std::size_t first,
                                    std::vector<CollisionPair>& pairs) {
    const int centerCell = collisionCellIndex(grid, bubbles[first].position);
    const int centerRow = centerCell / grid.columns;
    const int centerColumn = centerCell - centerRow * grid.columns;

    for (int rowOffset = -1; rowOffset <= 1; ++rowOffset) {
        const int row = centerRow + rowOffset;
        if (row < 0 || row >= grid.rows) {
            continue;
        }
        for (int columnOffset = -1; columnOffset <= 1; ++columnOffset) {
            const int column = centerColumn + columnOffset;
            if (column < 0 || column >= grid.columns) {
                continue;
            }

            int candidate = grid.heads[static_cast<std::size_t>(
                row * grid.columns + column)];
            while (candidate >= 0) {
                const std::size_t second = static_cast<std::size_t>(candidate);
                if (second > first) {
                    const float dx = bubbles[second].position.x - bubbles[first].position.x;
                    const float dy = bubbles[second].position.y - bubbles[first].position.y;
                    const float minimumDistance = bubbles[first].radius + bubbles[second].radius;
                    if (dx * dx + dy * dy < minimumDistance * minimumDistance) {
                        pairs.push_back({first, second});
                    }
                }
                candidate = grid.next[second];
            }
        }
    }
}

// Separa cada par y aplica un impulso elástico según el área de los círculos.
void resolveCollisionPairs(std::vector<Bubble>& bubbles,
                           const std::vector<CollisionPair>& pairs,
                           float areaWidth,
                           float areaHeight) {
    for (const CollisionPair& pair : pairs) {
        Bubble& first = bubbles[pair.first];
        Bubble& second = bubbles[pair.second];
        const float dx = second.position.x - first.position.x;
        const float dy = second.position.y - first.position.y;
        const float minimumDistance = first.radius + second.radius;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared >= minimumDistance * minimumDistance) {
            continue;
        }

        float normalX = 1.0F;
        float normalY = 0.0F;
        float distance = 0.0F;
        if (distanceSquared > std::numeric_limits<float>::epsilon()) {
            distance = std::sqrt(distanceSquared);
            normalX = dx / distance;
            normalY = dy / distance;
        } else if (((pair.first + pair.second) & 1U) != 0U) {
            normalX = 0.0F;
            normalY = 1.0F;
        }

        const float inverseMassFirst = 1.0F / std::max(first.radius * first.radius, 0.01F);
        const float inverseMassSecond = 1.0F / std::max(second.radius * second.radius, 0.01F);
        const float inverseMassSum = inverseMassFirst + inverseMassSecond;
        // Una correccion levemente mayor evita que contactos multiples vuelvan
        // a introducir una superposicion visible en la misma iteracion.
        const float penetration = (minimumDistance - distance) * 1.10F
            + kCollisionSeparationEpsilon;

        first.position.x -= normalX * penetration * inverseMassFirst / inverseMassSum;
        first.position.y -= normalY * penetration * inverseMassFirst / inverseMassSum;
        second.position.x += normalX * penetration * inverseMassSecond / inverseMassSum;
        second.position.y += normalY * penetration * inverseMassSecond / inverseMassSum;

        const float relativeVelocityX = second.velocity.x - first.velocity.x;
        const float relativeVelocityY = second.velocity.y - first.velocity.y;
        const float velocityAlongNormal = relativeVelocityX * normalX
            + relativeVelocityY * normalY;
        if (velocityAlongNormal < 0.0F) {
            const float impulse = -(1.0F + kCollisionRestitution)
                * velocityAlongNormal / inverseMassSum;
            first.velocity.x -= impulse * inverseMassFirst * normalX;
            first.velocity.y -= impulse * inverseMassFirst * normalY;
            second.velocity.x += impulse * inverseMassSecond * normalX;
            second.velocity.y += impulse * inverseMassSecond * normalY;
        }
    }

    for (Bubble& bubble : bubbles) {
        reflectAtBorders(bubble.position.x, bubble.velocity.x, bubble.radius, areaWidth);
        reflectAtBorders(bubble.position.y, bubble.velocity.y, bubble.radius, areaHeight);
    }
}

int calculateCollisionSubsteps(const std::vector<Bubble>& bubbles, float deltaTime) {
    float minimumRadius = std::numeric_limits<float>::max();
    float maximumSpeed = 0.0F;
    for (const Bubble& bubble : bubbles) {
        minimumRadius = std::min(minimumRadius, bubble.radius);
        maximumSpeed = std::max(maximumSpeed, std::sqrt(
            bubble.velocity.x * bubble.velocity.x + bubble.velocity.y * bubble.velocity.y));
    }
    const float targetTravel = std::max(0.5F * minimumRadius, 0.25F);
    const int required = static_cast<int>(std::ceil(maximumSpeed * deltaTime / targetTravel));
    return std::clamp(required, 1, kMaximumCollisionSubsteps);
}

void resolveCollisionsSequential(std::vector<Bubble>& bubbles,
                                 float areaWidth,
                                 float areaHeight,
                                 CollisionGrid& grid,
                                 std::vector<CollisionPair>& pairs) {
    for (int iteration = 0; iteration < kCollisionSolverIterations; ++iteration) {
        buildCollisionGrid(bubbles, areaWidth, areaHeight, grid);
        pairs.clear();
        for (std::size_t first = 0; first < bubbles.size(); ++first) {
            collectCollisionPairsForBubble(bubbles, grid, first, pairs);
        }
        if (pairs.empty()) {
            break;
        }
        std::sort(pairs.begin(), pairs.end(), collisionPairLess);
        resolveCollisionPairs(bubbles, pairs, areaWidth, areaHeight);
    }
}

void advancePhysicsSequential(std::vector<Bubble>& bubbles,
                              float safeDelta,
                              float areaWidth,
                              float areaHeight,
                              CollisionGrid& grid,
                              std::vector<CollisionPair>& pairs) {
    const int substeps = calculateCollisionSubsteps(bubbles, safeDelta);
    const float substepDelta = safeDelta / static_cast<float>(substeps);
    for (int substep = 0; substep < substeps; ++substep) {
        for (Bubble& bubble : bubbles) {
            updateSingleBubble(bubble, substepDelta, areaWidth, areaHeight);
        }
        resolveCollisionsSequential(
            bubbles, areaWidth, areaHeight, grid, pairs);
    }
}

}  // namespace detail

void updatePhysics(std::vector<Bubble>& bubbles,
                   double deltaTime,
                   float areaWidth,
                   float areaHeight) {
    // La referencia secuencial mueve, detecta y resuelve las colisiones.
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0
        || !std::isfinite(areaWidth) || !std::isfinite(areaHeight)
        || areaWidth <= 0.0F || areaHeight <= 0.0F) {
        return;
    }

    const float safeDelta = static_cast<float>(std::min(deltaTime, kMaximumDeltaTime));
    detail::CollisionGrid grid;
    std::vector<detail::CollisionPair> pairs;
    pairs.reserve(bubbles.size());
    detail::advancePhysicsSequential(
        bubbles, safeDelta, areaWidth, areaHeight, grid, pairs);
}

void updatePhysicsSequentialPersistent(std::vector<Bubble>& bubbles,
                                       std::uint64_t steps,
                                       double deltaTime,
                                       float areaWidth,
                                       float areaHeight) {
    if (steps == 0
        || !std::isfinite(deltaTime) || deltaTime <= 0.0
        || !std::isfinite(areaWidth) || !std::isfinite(areaHeight)
        || areaWidth <= 0.0F || areaHeight <= 0.0F) {
        return;
    }

    const float safeDelta = static_cast<float>(std::min(deltaTime, kMaximumDeltaTime));
    detail::CollisionGrid grid;
    std::vector<detail::CollisionPair> pairs;
    pairs.reserve(bubbles.size());
    for (std::uint64_t step = 0; step < steps; ++step) {
        detail::advancePhysicsSequential(
            bubbles, safeDelta, areaWidth, areaHeight, grid, pairs);
    }
}

}  // namespace bubbles
