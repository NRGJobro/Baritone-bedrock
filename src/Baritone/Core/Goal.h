#pragma once

#include "BlockPos.h"

#include <memory>
#include <string>
#include <vector>

namespace baritone {

class Goal {
public:
    virtual ~Goal() = default;
    [[nodiscard]] virtual bool isInGoal(const BlockPos& pos) const = 0;
    [[nodiscard]] virtual double heuristic(const BlockPos& pos) const = 0;
    [[nodiscard]] virtual std::string describe() const = 0;
};

class GoalBlock final : public Goal {
    BlockPos target;

public:
    explicit GoalBlock(BlockPos target);
    [[nodiscard]] bool isInGoal(const BlockPos& pos) const override;
    [[nodiscard]] double heuristic(const BlockPos& pos) const override;
    [[nodiscard]] std::string describe() const override;
    [[nodiscard]] const BlockPos& getTarget() const;
};

class GoalXZ final : public Goal {
    int x;
    int z;

public:
    GoalXZ(int x, int z);
    [[nodiscard]] bool isInGoal(const BlockPos& pos) const override;
    [[nodiscard]] double heuristic(const BlockPos& pos) const override;
    [[nodiscard]] std::string describe() const override;
    [[nodiscard]] int getX() const;
    [[nodiscard]] int getZ() const;
};

class GoalYLevel final : public Goal {
    int y;

public:
    explicit GoalYLevel(int y);
    [[nodiscard]] bool isInGoal(const BlockPos& pos) const override;
    [[nodiscard]] double heuristic(const BlockPos& pos) const override;
    [[nodiscard]] std::string describe() const override;
    [[nodiscard]] int getY() const;
};

class GoalNear final : public Goal {
    BlockPos target;
    int radius;

public:
    GoalNear(BlockPos target, int radius);
    [[nodiscard]] bool isInGoal(const BlockPos& pos) const override;
    [[nodiscard]] double heuristic(const BlockPos& pos) const override;
    [[nodiscard]] std::string describe() const override;
    [[nodiscard]] const BlockPos& getTarget() const;
    [[nodiscard]] int getRadius() const;
};

class GoalComposite final : public Goal {
    std::vector<std::shared_ptr<Goal>> goals;

public:
    explicit GoalComposite(std::vector<std::shared_ptr<Goal>> goals);
    [[nodiscard]] bool isInGoal(const BlockPos& pos) const override;
    [[nodiscard]] double heuristic(const BlockPos& pos) const override;
    [[nodiscard]] std::string describe() const override;
};

} // namespace baritone
