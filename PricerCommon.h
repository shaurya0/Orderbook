#pragma once
#include <stdint.h>
#include <iostream>

namespace Pricer
{
    class Utils
    {
    public:
        static uint64_t convert_price( double price )
        {
            return static_cast<uint64_t>( price*100 );
        }

        static double convert_price( uint64_t price )
        {
            return static_cast<double>( price )/100.0;
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
