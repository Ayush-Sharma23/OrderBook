#ifndef DECL_H
#define DECL_H
#include <bits/stdc++.h>

using Price = int;
using Quantity = int;
using OrderId = int;
using PoolIdx = uint32_t;

enum Type{
    Buy,
    Sell
};

const Price MIN_PRICE = 1;
const Price MAX_PRICE = 1000;
const size_t ARRAY_SIZE = MAX_PRICE - MIN_PRICE + 1;

const PoolIdx INVALID_IDX = 0xFFFFFFFF;
const size_t MAX_ORDERS = 1005000;
const size_t MAX_ORDER_IDS = 1005000;

#endif

