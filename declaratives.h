#ifndef DECL_H
#define DECL_H

#include <cstdint>
#include <cstddef>

using Price = int32_t;
using Quantity = int32_t;
using OrderId = int32_t;
using PoolIdx = uint32_t;

enum class Type: uint8_t{
    Buy,
    Sell
};

const Price MIN_PRICE = 1;
const Price MAX_PRICE = 1000;
const size_t ARRAY_SIZE = MAX_PRICE - MIN_PRICE + 1;

const PoolIdx INVALID_IDX = 0xFFFFFFFF;
const size_t MAX_ORDERS = 2005000;
const size_t MAX_ORDER_IDS = 2005000;

#endif

