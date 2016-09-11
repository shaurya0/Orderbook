#pragma once
#include <stdint.h>
#include <iostream>

namespace Pricer
{
    class Utils
    {
    public:
        static uint32_t convert_price( float price )
        {
            return static_cast<uint32_t>( price*100 );
        }

        static float convert_price( uint32_t price )
        {
            return static_cast<float>( price )/100.0f;
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
		default:
			break;
        }
    }
}
