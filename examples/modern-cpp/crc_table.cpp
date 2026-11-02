#include <array>
#include <cstdint>
#include <span>

#include "print.h"

/* CRC-16/ARC: polynomial 0xA001 in the reflected form. The table is computed while the program is
   compiled and stored in flash as constant data. */
constexpr std::array<std::uint16_t, 256> make_crc16_table() noexcept
{
    std::array<std::uint16_t, 256> table{};
    for (unsigned i = 0; i < table.size(); ++i) {
        auto crc = static_cast<std::uint16_t>(i);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1u) != 0u ? static_cast<std::uint16_t>((crc >> 1) ^ 0xA001u)
                                   : static_cast<std::uint16_t>(crc >> 1);
        }
        table[i] = crc;
    }
    return table;
}

inline constexpr auto crc16_table = make_crc16_table();
static_assert(crc16_table[1] == 0xC0C1, "the second entry of the CRC-16/ARC table");

std::uint16_t crc16(std::span<const std::uint8_t> data) noexcept
{
    std::uint16_t crc = 0;
    for (const std::uint8_t byte : data) {
        crc = static_cast<std::uint16_t>((crc >> 8) ^ crc16_table[(crc ^ byte) & 0xFFu]);
    }
    return crc;
}

int main()
{
    static constexpr std::array<std::uint8_t, 9> check{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    print_u32("crc16 of 123456789", crc16(check));
    plat_exit(0);
}
