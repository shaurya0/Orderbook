#pragma once
#include "PricerCommon.h"
#include "OrderParser.h"
#include "OrderBook.h"
#include "OrderBookManager.h"
#include <iomanip>
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
		enum class OrderState {FULFILLED, NOT_FULFILLED};
		struct PricingState
		{
			const uint32_t target_size;
			uint32_t last_result;
			uint32_t worst_price;
            uint32_t num_orders;
			OrderState previous_state;
		};

		const OrderBookManager &_order_book_manager;
		PricingState _buy_state;
		PricingState _sell_state;

		// TODO: refactor
		template<typename OB>
		static std::pair<uint32_t, uint32_t> order_book_result(const OB& order_book, PricingState &pricing_state) noexcept
		{
			uint32_t result = 0;
			uint32_t num_shares = 0;
			for (const auto &levels : order_book.get_orders())
			{
				uint32_t price = levels.first;
				for (const auto &levels : levels.second)
				{
					uint32_t shares = 0;
					if (levels.second + num_shares > pricing_state.target_size)
					{
						shares = pricing_state.target_size - num_shares;
					}
					else
					{
						shares = levels.second;
					}
					result += price*shares;
					num_shares += shares;

					if (shares > 0)
						pricing_state.worst_price = price;

					if(num_shares == pricing_state.target_size)
						return std::make_pair(result, num_shares);
				}
			}
			return std::make_pair(result, num_shares);
		}

        enum class result_type {INCOME, EXPENSES};
        void compute_pricer_result( const Order &order, result_type rt )
        {
            PricingState *pricing_state = nullptr;
			std::pair<uint32_t, uint32_t> result_shares = std::make_pair(0, 0);
            char msg;
            uint32_t total_orders = 0;
            if (rt == result_type::INCOME)
            {
                msg = 'S';
				pricing_state = &_sell_state;
                total_orders = _order_book_manager.get_bid_order_book().get_total_orders();
            }
            else
            {
                msg = 'B';
				pricing_state = &_buy_state;
                total_orders = _order_book_manager.get_ask_order_book().get_total_orders();
            }

			bool compute = true;
			if (total_orders < pricing_state->target_size)
			{
				const bool state_changed = pricing_state->previous_state == OrderState::FULFILLED;
				if (state_changed)
				{
					std::cout << order.milliseconds << " " << msg << " NA" << std::endl;
				}
				pricing_state->previous_state = OrderState::NOT_FULFILLED;
				compute = false;
			}


			if (compute)
			{
				if( rt == result_type::INCOME )
					result_shares = order_book_result(_order_book_manager.get_bid_order_book(), *pricing_state);
				else
					result_shares = order_book_result(_order_book_manager.get_ask_order_book(), *pricing_state);

				const bool result_changed = result_shares.first != pricing_state->last_result;
				if ( result_changed)
				{
					float result = Utils::convert_price(result_shares.first);
					std::cout << order.milliseconds << " " << msg << " " << std::setprecision(2) << std::fixed << result << std::endl;
					pricing_state->previous_state = OrderState::FULFILLED;
				}
			}

            pricing_state->last_result = result_shares.first;
            pricing_state->num_orders = total_orders;
        }

		void compute_expenses(const Order &order)
		{
            compute_pricer_result( order, result_type::EXPENSES );
        }

        void compute_income(const Order &order)
        {
            compute_pricer_result( order, result_type::INCOME );
        }


    public:
		PricingEngine(uint32_t target_size, OrderBookManager &manager)
			:
			_buy_state({ target_size, 0, std::numeric_limits<uint32_t>::max(), 0, OrderState::NOT_FULFILLED })
			, _sell_state({ target_size, 0, std::numeric_limits<uint32_t>::max(), 0, OrderState::NOT_FULFILLED })
			, _order_book_manager(manager)
		{
			manager.get_ask_order_book().register_observer([&](const Pricer::Order& order) { compute_expenses(order); });
			manager.get_bid_order_book().register_observer([&](const Pricer::Order& order) { compute_income(order); });
		}
	};
} // Pricer
