#pragma once

#include "bubbles/types.hpp"

#include <cstddef>
#include <vector>

namespace bubbles {
namespace detail {

struct CollisionPair {
    std::size_t first;
    std::size_t second;
};

struct CollisionGrid {
    float cellSize = 1.0F;
    int columns = 1;
    int rows = 1;
    std::vector<int> heads;
    std::vector<int> next;
};

void updateSingleBubble(Bubble& bubble, float deltaTime, float areaWidth, float areaHeight);
bool collisionPairLess(const CollisionPair& left, const CollisionPair& right);
int collisionCellIndex(const CollisionGrid& grid, const Vec2& position);
void buildCollisionGrid(const std::vector<Bubble>& bubbles, float areaWidth, float areaHeight, CollisionGrid& grid);
void collectCollisionPairsForBubble(const std::vector<Bubble>& bubbles, const CollisionGrid& grid, std::size_t first, std::vector<CollisionPair>& pairs);
void resolveCollisionPairs(std::vector<Bubble>& bubbles, const std::vector<CollisionPair>& pairs, float areaWidth, float areaHeight);
int calculateCollisionSubsteps(
    const std::vector<Bubble>& bubbles,
    float deltaTime);

}  // namespace detail
}  // namespace bubbles
