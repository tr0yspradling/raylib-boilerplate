#pragma once

#include <cstdint>
#include <functional>

namespace shared::game {

template <typename Tag>
class Id {
public:
    constexpr Id() = default;
    explicit constexpr Id(uint32_t value) : value_(value) {}

    [[nodiscard]] constexpr uint32_t Value() const { return value_; }
    [[nodiscard]] constexpr bool IsValid() const { return value_ != 0; }

    friend constexpr bool operator==(const Id&, const Id&) = default;
    friend constexpr auto operator<=>(const Id&, const Id&) = default;

private:
    uint32_t value_ = 0;
};

struct EntityIdTag;
struct PlayerIdTag;

using EntityId = Id<EntityIdTag>;
using PlayerId = Id<PlayerIdTag>;
using TickId = uint32_t;

template <typename Tag>
struct IdHash {
    std::size_t operator()(Id<Tag> value) const noexcept {
        return std::hash<uint32_t>{}(value.Value());
    }
};

}  // namespace shared::game
