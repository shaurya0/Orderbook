#pragma once
#include "PricerCommon.h"
#include "OrderParser.h"
#include "OrderBook.h"
#include "OrderBookController.h"
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <atomic>
#include <utility>
#include <string>
#include <unordered_map>
#include <numeric>
#include <deque>
#include <list>
#include <map>

namespace Pricer
{
	class PricingEngine
	{
	private:
		struct BuyState
		{
			const uint32_t target_size;
			uint64_t last_expense;
			uint64_t most_expensive_share;
			bool emit_messages;
		};

		struct SellState
		{
			const uint32_t target_size;
			uint64_t last_income;
			uint64_t cheapest_share;
			bool emit_messages;
		};

		const OrderBookController &_order_book_controller;
		BuyState _buy_state;
		SellState _sell_state;

		template<typename OB, typename OBState>
		static std::pair<uint64_t, uint64_t> order_book_target(const OB& order_book, OBState &ob_state) noexcept
		{
			uint64_t result = 0;
			uint64_t num_shares = 0;
			for (const auto &levels : order_book.get_orders())
			{
				uint64_t price = levels.first;
				for (const auto &levels : levels.second)
				{
					uint64_t shares = 0;
					if (levels.second + num_shares > ob_state.target_size)
					{
						shares = ob_state.target_size - num_shares;
					}
					else
					{
						shares = levels.second;
					}
					result += price*shares;
					num_shares += shares;
				}
			}
			return std::make_pair(result, num_shares);
		}


		void compute_income(const Order &order)
		{
			auto income_shares = order_book_target(_order_book_controller.get_ask_order_book(), _sell_state);
			if (income_shares.second == _sell_state.target_size && income_shares.first != _sell_state.last_income)
			{
				_sell_state.emit_messages = true;
				double income = Utils::convert_price(income_shares.first);
				std::cout << order.milliseconds << " B " << income << std::endl;
			}
			else
			{
				if (_sell_state.emit_messages && income_shares.first != _sell_state.last_income)
				{
					std::cout << order.milliseconds << " B na" << std::endl;
				}
			}
			_sell_state.last_income = income_shares.first;
		}

		void compute_expenses(const Order &order)
		{
			auto expenses_shares = order_book_target(_order_book_controller.get_bid_order_book(), _buy_state);
			if (expenses_shares.second == _buy_state.target_size && expenses_shares.first != _buy_state.last_expense)
			{
				_buy_state.emit_messages = true;
				double expenses = Utils::convert_price(expenses_shares.first);
				std::cout << order.milliseconds << " S " << expenses << std::endl;
			}
			else
			{
				if (_buy_state.emit_messages && expenses_shares.first != _buy_state.last_expense)
				{
					std::cout << order.milliseconds << " S NA" << std::endl;
				}
			}
			_buy_state.last_expense = expenses_shares.first;
		}


	public:
		PricingEngine(uint32_t target_size, OrderBookController &controller)
			:
			_buy_state({ target_size, 0, 0, false })
			, _sell_state({ target_size, 0 , 0, false })
			, _order_book_controller(controller)
		{
			controller.get_ask_order_book().register_observer( [&](const Pricer::Order& order) { compute_income(order); } );
			controller.get_bid_order_book().register_observer( [&](const Pricer::Order& order) { compute_expenses(order); } );
		}

		void initialize()
		{

		}

		// optimization oppoprtunities:
		// cache the smallest bid in the sell : if a new bid comes in which is smaller and the previous sell was fulfilled then we don't need to recompute
		// cache the largest ask in the buy : if a new sell comes in which is larger and the previous buy was fulfilled then we don't need to recompute

	};
} // Pricer
