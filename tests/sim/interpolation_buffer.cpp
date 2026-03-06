#include <cassert>

#include "shared/game/interpolation.hpp"

int main() {
    using namespace shared::game;

    PositionInterpolationBuffer buffer;
    assert(buffer.Size() == 0);
    assert(buffer.SampleAt(0.0f).x == 0.0f);

    buffer.Push(PositionSample{.tick = 10, .position = {0.0f, 0.0f}});
    buffer.Push(PositionSample{.tick = 12, .position = {12.0f, 0.0f}});
    assert(buffer.Size() == 2);

    const Vec2f halfway = buffer.SampleAt(11.0f);
    assert(halfway.x > 5.9f && halfway.x < 6.1f);

    // Out-of-order sample should be ignored.
    buffer.Push(PositionSample{.tick = 11, .position = {99.0f, 0.0f}});
    assert(buffer.Size() == 2);
    const Vec2f stillHalfway = buffer.SampleAt(11.0f);
    assert(stillHalfway.x > 5.9f && stillHalfway.x < 6.1f);

    // Same tick replaces latest sample.
    buffer.Push(PositionSample{.tick = 12, .position = {24.0f, 0.0f}});
    assert(buffer.Size() == 2);
    assert(buffer.SampleAt(12.0f).x == 24.0f);

    buffer.Clear();
    assert(buffer.Size() == 0);

    return 0;
}
