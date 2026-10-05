// Copyright 2026 Slick Quant
// SPDX-License-Identifier: MIT

#include <slick/orderbook/orderbook_manager.hpp>

#include <slick/orderbook/detail/impl/orderbook_manager_impl.hpp>

SLICK_NAMESPACE_BEGIN

// Explicit template instantiations for compiled library mode
SLICK_DLL_INTERFACE_WARNINGS_PUSH
template class SLICK_TEMPLATE_INSTANTIATION_API OrderBookManager<OrderBookL2>;
template class SLICK_TEMPLATE_INSTANTIATION_API OrderBookManager<OrderBookL3>;
SLICK_DLL_INTERFACE_WARNINGS_POP

SLICK_NAMESPACE_END
