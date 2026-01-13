#pragma once

#include <vector>
#include <cstdint>

namespace esphome {
namespace levoit {

// Message counter for tracking sent messages
inline std::uint8_t messageUpCounter = 16;

constexpr size_t LEVOIT_IDX_COUNTER = 2;
constexpr size_t LEVOIT_IDX_LENGTH = 3;
constexpr size_t LEVOIT_IDX_CHECKSUM = 5;

/// @brief Calculates checksum for Levoit messages
inline uint8_t levoit_checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (i == 5)
            continue; // skip checksum byte
        sum += data[i];
    }
    return (uint8_t)(0xFF - (sum & 0xFF));
}

/// @brief Adds counter and checksum to a Levoit message
inline void levoit_finalize_message(std::vector<uint8_t> &message, uint8_t counter)
{
    // Sanity check
    if (message.size() <= LEVOIT_IDX_CHECKSUM)
        return;

    // Set message counter
    message[LEVOIT_IDX_COUNTER] = counter;

    // Compute checksum (skip checksum byte itself)
    uint32_t sum = 0;
    for (size_t i = 0; i < message.size(); i++)
    {
        if (i == LEVOIT_IDX_CHECKSUM)
            continue;
        sum += message[i];
    }

    message[LEVOIT_IDX_CHECKSUM] = static_cast<uint8_t>(0xFF - (sum & 0xFF));
}

/// @brief Builds a complete Levoit protocol message
inline std::vector<uint8_t> build_levoit_message(
    const std::vector<uint8_t> &msg_type,
    const std::vector<uint8_t> &payload,
    uint8_t counter)
{
    std::vector<uint8_t> msg;

    // ---- header (checksum placeholder at [5]) ----
    msg.push_back(0xA5);                                     // 0
    msg.push_back(0x22);                                     // 1
    msg.push_back(counter);                                  // 2
    msg.push_back(0xFF);                                     // 3 length (filled later)
    msg.push_back(0x00);                                     // 4 fixed ´0x00´
    msg.push_back(0xFF);                                     // 5 checksum placeholder
    msg.insert(msg.end(), msg_type.begin(), msg_type.end()); // 6,7,8
    msg.push_back(0x00);                                     // 9
    // ---- payload ----
    if (!payload.empty())
        msg.insert(msg.end(), payload.begin(), payload.end());

    // ---- length = bytes AFTER checksum ----
    msg[LEVOIT_IDX_LENGTH] = static_cast<uint8_t>(msg.size() - (LEVOIT_IDX_CHECKSUM + 1));

    // ---- checksum ----
    levoit_finalize_message(msg, counter);

    return msg;
}

}  // namespace levoit
}  // namespace esphome

