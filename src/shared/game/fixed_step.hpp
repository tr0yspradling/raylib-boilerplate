#pragma once

#include <algorithm>

#include "shared/game/game_policy.hpp"

namespace shared::game {

class FixedStep {
public:
    explicit FixedStep(double stepSeconds) : stepSeconds_(stepSeconds) {}

    [[nodiscard]] int Accumulate(double frameSeconds) {
        accumulatorSeconds_ += std::max(frameSeconds, 0.0);

        int stepCount = 0;
        while (accumulatorSeconds_ >= stepSeconds_ && stepCount < maxCatchupSteps_) {
            accumulatorSeconds_ -= stepSeconds_;
            ++stepCount;
        }

        if (stepCount == maxCatchupSteps_ && accumulatorSeconds_ > stepSeconds_) {
            accumulatorSeconds_ = stepSeconds_;
        }

        return stepCount;
    }

    [[nodiscard]] double StepSeconds() const { return stepSeconds_; }
    [[nodiscard]] double InterpolationAlpha() const { return accumulatorSeconds_ / stepSeconds_; }

    void SetMaxCatchupSteps(int maxCatchupSteps) { maxCatchupSteps_ = std::max(1, maxCatchupSteps); }
    void SetStepSeconds(double stepSeconds) {
        stepSeconds_ = std::max(stepSeconds, policy::fixed_step::kMinStepSeconds);
        accumulatorSeconds_ = std::min(accumulatorSeconds_, stepSeconds_);
    }

private:
    double stepSeconds_ = policy::fixed_step::kDefaultStepSeconds;
    double accumulatorSeconds_ = 0.0;
    int maxCatchupSteps_ = policy::fixed_step::kDefaultMaxCatchupSteps;
};

}  // namespace shared::game
