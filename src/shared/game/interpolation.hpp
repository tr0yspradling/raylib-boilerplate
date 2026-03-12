#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>

#include "shared/game/game_policy.hpp"
#include "shared/game/ids.hpp"
#include "shared/game/math_types.hpp"

namespace shared::game {

struct PositionSample {
    TickId tick = 0;
    Vec2f position{};
};

class PositionInterpolationBuffer {
public:
    explicit PositionInterpolationBuffer(size_t maxSamples = policy::interpolation::kDefaultMaxSamples) :
        maxSamples_(std::max(policy::interpolation::kMinSamples, maxSamples)) {}

    void Push(PositionSample sample) {
        if (samples_.empty()) {
            samples_.push_back(sample);
            return;
        }

        if (sample.tick < samples_.back().tick) {
            // Drop out-of-order samples. Snapshot lanes are unreliable.
            return;
        }

        if (sample.tick == samples_.back().tick) {
            samples_.back() = sample;
            return;
        }

        samples_.push_back(sample);
        while (samples_.size() > maxSamples_) {
            samples_.pop_front();
        }
    }

    [[nodiscard]] Vec2f SampleAt(float renderTick) const {
        if (samples_.empty()) {
            return {};
        }

        if (samples_.size() == 1 || renderTick <= static_cast<float>(samples_.front().tick)) {
            return samples_.front().position;
        }

        for (size_t index = 1; index < samples_.size(); ++index) {
            const PositionSample& older = samples_[index - 1];
            const PositionSample& newer = samples_[index];
            if (renderTick <= static_cast<float>(newer.tick)) {
                const float span = static_cast<float>(newer.tick - older.tick);
                if (span <= 0.0f) {
                    return newer.position;
                }

                const float alpha = (renderTick - static_cast<float>(older.tick)) / span;
                return Lerp(older.position, newer.position, alpha);
            }
        }

        return samples_.back().position;
    }

    void Clear() { samples_.clear(); }
    [[nodiscard]] size_t Size() const { return samples_.size(); }

private:
    size_t maxSamples_ = policy::interpolation::kDefaultMaxSamples;
    std::deque<PositionSample> samples_;
};

}  // namespace shared::game
