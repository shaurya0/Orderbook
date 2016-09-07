#pragma once
#include <map>
#include <stdint.h>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>

namespace Pricer
{
	enum class ADD_ORDER_TYPE{ BID, ASK };
	enum class ORDER_TYPE{ ADD, REDUCE };
	struct AddOrder
	{
		ADD_ORDER_TYPE order_type;
		double limit_price;
	};

	struct ReduceOrder{};

	static std::map<char, ADD_ORDER_TYPE> char_to_add_order = { { 'B', ADD_ORDER_TYPE::BID }, { 'A', ADD_ORDER_TYPE::ASK } };
	static std::map<char, ORDER_TYPE> char_to_order = { { 'A', ORDER_TYPE::ADD }, { 'R', ORDER_TYPE::REDUCE } };

	struct Order
	{
		ORDER_TYPE type;
		uint32_t milliseconds;
		char id;
		uint32_t size;

		boost::variant<AddOrder, ReduceOrder> _order;
	};


	class OrderParser
	{
	private:
		using tokenizer = boost::tokenizer<boost::char_separator<char>>;
		using token_iterator = tokenizer::iterator;
	public:
		static Order parse_order(const std::string &order_str)
		{
			boost::char_separator<char> sep{ " " };
			tokenizer tok{ order_str, sep };
			auto it = tok.begin();

			Order order;

			order.milliseconds = boost::lexical_cast<decltype(order.milliseconds)>(*it);
			std::advance(it, 1);

			const char order_type = boost::lexical_cast<char>(*it);
			order.type = char_to_order.at(order_type);

			std::advance(it, 1);

			order.id = boost::lexical_cast<decltype(order.id)>(*it);
			std::advance(it, 1);

			switch (order.type)
			{
			case ORDER_TYPE::ADD:
			{
				const char add_order_type_c = boost::lexical_cast<char>(*it);
				std::advance(it, 1);

				const auto add_order_type = char_to_add_order.at(add_order_type_c);
				const double limit_price = boost::lexical_cast<double>(*it);

				order._order = AddOrder
				{
					add_order_type,
					limit_price
				};
				break;
			}
			case ORDER_TYPE::REDUCE:
				order.size = boost::lexical_cast<decltype(order.size)>(*it);
				order._order = ReduceOrder{};
				break;
			}
			return order;
		}
	};

}