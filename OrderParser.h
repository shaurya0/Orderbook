#pragma once
#include <map>
#include <stdint.h>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>
#include <string>
#include "PricerCommon.h"

namespace Pricer
{
	enum class ADD_ORDER_TYPE{ BID, ASK };
	enum class ORDER_TYPE{ ADD, REDUCE };

	struct AddOrder
	{
		ADD_ORDER_TYPE order_type;
		uint32_t limit_price;
	};

	struct ReduceOrder
	{
		uint32_t price;
	};


	struct Order
	{
		static std::map<char, ADD_ORDER_TYPE> char_to_add_order;
		static std::map<char, ORDER_TYPE> char_to_order;
		ORDER_TYPE type;
		uint32_t milliseconds;
		std::string id;
		uint32_t size;

		boost::variant<AddOrder, ReduceOrder> _order;
	};

	std::map<char, ADD_ORDER_TYPE> Order::char_to_add_order = { { 'B', ADD_ORDER_TYPE::BID }, { 'S', ADD_ORDER_TYPE::ASK } };
	std::map<char, ORDER_TYPE> Order::char_to_order = { { 'A', ORDER_TYPE::ADD }, { 'R', ORDER_TYPE::REDUCE } };

	class OrderParser
	{
	private:
		using tokenizer = boost::tokenizer<boost::char_separator<char>>;
		using token_iterator = tokenizer::iterator;

		template<typename Target>
		static bool try_parse_token( token_iterator &token, Target& result )
		{
			if (boost::conversion::try_lexical_convert(*token, result))
			{
				++token;
				return true;
			}

			return false;
		}

	public:
		static Pricer::ErrorCode parse_order(const std::string &order_str, Order &order)
		{
			if (order_str.empty())
				return ErrorCode::PARSE_FAILED;
			boost::char_separator<char> sep{ " " };
			tokenizer tok{ order_str, sep };
			auto it = tok.begin();

			if (!try_parse_token(it, order.milliseconds))
				return ErrorCode::PARSE_FAILED;

			char order_type;
			if (!try_parse_token(it, order_type))
				return ErrorCode::PARSE_FAILED;

			if( Order::char_to_order.end() == Order::char_to_order.find(order_type) )
				return ErrorCode::PARSE_FAILED;

			order.type = Order::char_to_order.at(order_type);


			if (!try_parse_token(it, order.id))
				return ErrorCode::PARSE_FAILED;

			switch (order.type)
			{
			case ORDER_TYPE::ADD:
			{
				AddOrder add_order;

				char add_order_type_c;
				if (!try_parse_token(it, add_order_type_c))
					return ErrorCode::PARSE_FAILED;

				if( Order::char_to_add_order.end() == Order::char_to_add_order.find(add_order_type_c) )
					return ErrorCode::PARSE_FAILED;

				add_order.order_type = Order::char_to_add_order.at(add_order_type_c);

				float limit_price;
				if (!try_parse_token(it, limit_price))
					return ErrorCode::PARSE_FAILED;
				add_order.limit_price = Utils::convert_price(limit_price);

				order._order = add_order;

				if (!try_parse_token(it, order.size))
					return ErrorCode::PARSE_FAILED;

				break;
			}
			case ORDER_TYPE::REDUCE:
			{
				if (!try_parse_token(it, order.size))
					return ErrorCode::PARSE_FAILED;
				order._order = ReduceOrder{};
				break;
			}

			}

			return ErrorCode::NONE;
		}
	};

}
