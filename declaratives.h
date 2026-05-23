#ifndef DECL_H
#define DECL_H
#include <bits/stdc++.h>

using Price = int;
using Quantity = int;
using OrderId = int;

enum Type{
    Buy,
    Sell
};

const Price MIN_PRICE = 1;
const Price MAX_PRICE = 1000;
const size_t ARRAY_SIZE = MAX_PRICE - MIN_PRICE + 1;

#endif

