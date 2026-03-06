#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace shared::net {

class ByteWriter {
public:
    [[nodiscard]] const std::vector<uint8_t>& Data() const { return data_; }
    [[nodiscard]] std::vector<uint8_t> Take() && { return std::move(data_); }

    void WriteU8(uint8_t value) { data_.push_back(value); }

    void WriteBool(bool value) { WriteU8(value ? 1U : 0U); }

    void WriteU16(uint16_t value) {
        data_.push_back(static_cast<uint8_t>((value >> 8U) & 0xffU));
        data_.push_back(static_cast<uint8_t>(value & 0xffU));
    }

    void WriteU32(uint32_t value) {
        data_.push_back(static_cast<uint8_t>((value >> 24U) & 0xffU));
        data_.push_back(static_cast<uint8_t>((value >> 16U) & 0xffU));
        data_.push_back(static_cast<uint8_t>((value >> 8U) & 0xffU));
        data_.push_back(static_cast<uint8_t>(value & 0xffU));
    }

    void WriteU64(uint64_t value) {
        for (int shift = 56; shift >= 0; shift -= 8) {
            data_.push_back(static_cast<uint8_t>((value >> static_cast<uint32_t>(shift)) & 0xffU));
        }
    }

    void WriteI32(int32_t value) { WriteU32(static_cast<uint32_t>(value)); }

    void WriteFloat(float value) {
        static_assert(sizeof(float) == sizeof(uint32_t));
        WriteU32(std::bit_cast<uint32_t>(value));
    }

    bool WriteString(std::string_view value) {
        if (value.size() > std::numeric_limits<uint16_t>::max()) {
            return false;
        }

        WriteU16(static_cast<uint16_t>(value.size()));
        data_.insert(data_.end(), value.begin(), value.end());
        return true;
    }

    bool WriteBytes(std::span<const uint8_t> bytes) {
        if (bytes.size() > std::numeric_limits<uint32_t>::max()) {
            return false;
        }

        WriteU32(static_cast<uint32_t>(bytes.size()));
        data_.insert(data_.end(), bytes.begin(), bytes.end());
        return true;
    }

    void Append(std::span<const uint8_t> bytes) { data_.insert(data_.end(), bytes.begin(), bytes.end()); }

private:
    std::vector<uint8_t> data_;
};

class ByteReader {
public:
    explicit ByteReader(std::span<const uint8_t> bytes) : bytes_(bytes) {}

    [[nodiscard]] size_t Remaining() const { return bytes_.size() - offset_; }
    [[nodiscard]] bool IsDone() const { return offset_ == bytes_.size(); }

    bool ReadU8(uint8_t& value) {
        if (Remaining() < sizeof(uint8_t)) {
            return false;
        }

        value = bytes_[offset_++];
        return true;
    }

    bool ReadBool(bool& value) {
        uint8_t raw = 0;
        if (!ReadU8(raw)) {
            return false;
        }

        value = raw != 0;
        return true;
    }

    bool ReadU16(uint16_t& value) {
        if (Remaining() < sizeof(uint16_t)) {
            return false;
        }

        value = static_cast<uint16_t>(static_cast<uint16_t>(bytes_[offset_]) << 8U);
        value |= bytes_[offset_ + 1U];
        offset_ += sizeof(uint16_t);
        return true;
    }

    bool ReadU32(uint32_t& value) {
        if (Remaining() < sizeof(uint32_t)) {
            return false;
        }

        value = static_cast<uint32_t>(bytes_[offset_]) << 24U;
        value |= static_cast<uint32_t>(bytes_[offset_ + 1U]) << 16U;
        value |= static_cast<uint32_t>(bytes_[offset_ + 2U]) << 8U;
        value |= static_cast<uint32_t>(bytes_[offset_ + 3U]);
        offset_ += sizeof(uint32_t);
        return true;
    }

    bool ReadU64(uint64_t& value) {
        if (Remaining() < sizeof(uint64_t)) {
            return false;
        }

        value = 0;
        for (int index = 0; index < 8; ++index) {
            value = (value << 8U) | static_cast<uint64_t>(bytes_[offset_ + static_cast<size_t>(index)]);
        }

        offset_ += sizeof(uint64_t);
        return true;
    }

    bool ReadI32(int32_t& value) {
        uint32_t raw = 0;
        if (!ReadU32(raw)) {
            return false;
        }

        value = static_cast<int32_t>(raw);
        return true;
    }

    bool ReadFloat(float& value) {
        uint32_t raw = 0;
        if (!ReadU32(raw)) {
            return false;
        }

        value = std::bit_cast<float>(raw);
        return true;
    }

    bool ReadString(std::string& value, uint16_t maxSize = 1024) {
        uint16_t len = 0;
        if (!ReadU16(len)) {
            return false;
        }

        if (len > maxSize || Remaining() < len) {
            return false;
        }

        value.assign(reinterpret_cast<const char*>(bytes_.data() + offset_), len);
        offset_ += len;
        return true;
    }

    bool ReadBytes(std::span<const uint8_t>& out, uint32_t maxSize = 1024 * 1024) {
        uint32_t len = 0;
        if (!ReadU32(len)) {
            return false;
        }

        if (len > maxSize || Remaining() < len) {
            return false;
        }

        out = bytes_.subspan(offset_, len);
        offset_ += len;
        return true;
    }

    bool ReadFixedBytes(size_t len, std::span<const uint8_t>& out) {
        if (Remaining() < len) {
            return false;
        }

        out = bytes_.subspan(offset_, len);
        offset_ += len;
        return true;
    }

private:
    std::span<const uint8_t> bytes_{};
    size_t offset_ = 0;
};

}  // namespace shared::net
