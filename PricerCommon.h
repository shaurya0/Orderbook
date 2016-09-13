#pragma once
#include <stdint.h>
#include <iostream>

namespace Pricer
{
	constexpr uint32_t PRICE_SCALING = 100;
	constexpr float FPRICE_SCALING = 100.0;
    class Utils
    {
    public:
        static uint32_t convert_price( double price )
        {
            return static_cast<uint32_t>(price*FPRICE_SCALING);
        }

    };

    enum class ErrorCode
    {
        NONE,
        PARSE_FAILED,
        ADD_FAILED,
        REDUCE_FAILED
    };

    void print_error_code( const ErrorCode ec )
    {
        switch( ec )
        {
        case ErrorCode::PARSE_FAILED:
            std::cerr << "failed to parse order" << std::endl;
            break;

        case ErrorCode::ADD_FAILED:
            std::cerr << "failed to add order" << std::endl;
            break;

        case ErrorCode::REDUCE_FAILED:
            std::cerr << "failed to reduce order" << std::endl;
            break;
        }
    }
}
