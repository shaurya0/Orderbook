#pragma once
#include "PricerCommon.h"
#include "OrderParser.h"
#include "OrderBook.h"
#include "OrderBookController.h"
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
		struct BuyState
		{
			const uint32_t target_size;
			uint32_t last_expense;
			uint32_t worst_price;
            uint32_t num_orders;
			OrderState previous_state;
		};

		struct SellState
		{
			const uint32_t target_size;
			uint32_t last_income;
			uint32_t worst_price;
            uint32_t num_orders;
			OrderState previous_state;
		};

		const OrderBookController &_order_book_controller;
		BuyState _buy_state;
		SellState _sell_state;

		template<typename OB, typename OBState>
		static std::pair<uint32_t, uint32_t> order_book_target(const OB& order_book, OBState &ob_state) noexcept
		{
			uint32_t result = 0;
			uint32_t num_shares = 0;
			for (const auto &levels : order_book.get_orders())
			{
				uint32_t price = levels.first;
				for (const auto &levels : levels.second)
				{
					uint32_t shares = 0;
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

					if (shares > 0)
						ob_state.worst_price = price;

					if(num_shares == ob_state.target_size)
						return std::make_pair(result, num_shares);
				}
			}
			return std::make_pair(result, num_shares);
		}


		void compute_expenses(const Order &order)
		{
            auto expenses_shares = order_book_target(_order_book_controller.get_ask_order_book(), _buy_state);
            const bool fulfilled_order = expenses_shares.second == _buy_state.target_size;
            const bool result_changed = expenses_shares.first != _buy_state.last_expense;
            if ( fulfilled_order && result_changed)
            {
                float expenses = Utils::convert_price(expenses_shares.first);
                std::cout << order.milliseconds << " B " <<  expenses << std::endl;
                _buy_state.previous_state = OrderState::FULFILLED;
            }
            else
            {
                const bool state_changed = _buy_state.previous_state == OrderState::FULFILLED;
                if (state_changed)
                {
                    std::cout << order.milliseconds << " B NA" << std::endl;
                }
                _buy_state.previous_state = OrderState::NOT_FULFILLED;
            }
            _buy_state.last_expense = expenses_shares.first;
            _buy_state.num_orders = _order_book_controller.get_ask_order_book().get_total_orders();
        }

        void compute_income(const Order &order)
        {
			auto income_shares = order_book_target(_order_book_controller.get_bid_order_book(), _sell_state);
            const bool fulfilled_order = income_shares.second == _sell_state.target_size;
            const bool result_changed = income_shares.first != _sell_state.last_income;
			if (fulfilled_order && result_changed)
			{
				float income = Utils::convert_price(income_shares.first);
				std::cout << order.milliseconds << " S " <<  income << std::endl;
                _sell_state.previous_state = OrderState::FULFILLED;
            }
            else
            {
                const bool state_changed = _sell_state.previous_state == OrderState::FULFILLED;
                if (state_changed)
                {
                    std::cout << order.milliseconds << " S NA" << std::endl;
                }
                _sell_state.previous_state = OrderState::NOT_FULFILLED;
            }
            _sell_state.last_income = income_shares.first;
            _sell_state.num_orders = _order_book_controller.get_bid_order_book().get_total_orders();
        }


    public:
		PricingEngine(uint32_t target_size, OrderBookController &controller)
			:
			_buy_state({ target_size, 0, std::numeric_limits<uint32_t>::max(), 0, OrderState::NOT_FULFILLED })
			, _sell_state({ target_size, 0, std::numeric_limits<uint32_t>::max(), 0, OrderState::NOT_FULFILLED })
			, _order_book_controller(controller)
		{
			controller.get_ask_order_book().register_observer([&](const Pricer::Order& order) { compute_expenses(order); });
			controller.get_bid_order_book().register_observer([&](const Pricer::Order& order) { compute_income(order); });
		}

		void initialize()
		{

		}

		// optimization oppoprtunities:
		// cache the smallest bid in the sell : if a new bid comes in which is smaller and the previous sell was fulfilled then we don't need to recompute
		// cache the largest ask in the buy : if a new sell comes in which is larger and the previous buy was fulfilled then we don't need to recompute

	};
} // Pricer
