#pragma once

#include <algorithm>

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
        stepSeconds_ = std::max(stepSeconds, 1e-6);
        accumulatorSeconds_ = std::min(accumulatorSeconds_, stepSeconds_);
    }

private:
    double stepSeconds_ = 1.0 / 30.0;
    double accumulatorSeconds_ = 0.0;
    int maxCatchupSteps_ = 8;
};

}  // namespace shared::game
