#pragma once
#include <cstdint>

class SystemIdentification
{
    public:
        virtual ~SystemIdentification() = default;
        virtual std::uint32_t getSerialSize() = 0;
        virtual void getSerial(char* buffer, std::uint32_t bufflen) = 0;
};
