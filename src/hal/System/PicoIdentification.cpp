#include <PicoIdentification.hpp>
#include "pico/unique_id.h"

#include <cstdint>

std::uint32_t PicoIdentification::getSerialSize()
{
    return (PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1);
}

void PicoIdentification::getSerial(char* buffer, std::uint32_t bufflen)
{
    pico_get_unique_board_id_string(buffer, bufflen);
}