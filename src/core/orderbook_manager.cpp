// Copyright 2026 Slick Quant LLC
// SPDX-License-Identifier: MIT

#include <slick/orderbook/orderbook_manager.hpp>

#include <slick/orderbook/detail/impl/orderbook_manager_impl.hpp>

SLICK_NAMESPACE_BEGIN

// Explicit template instantiations for compiled library mode
template class OrderBookManager<OrderBookL2>;
template class OrderBookManager<OrderBookL3>;

SLICK_NAMESPACE_END
