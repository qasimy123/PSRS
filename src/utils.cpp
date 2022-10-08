/**
 * @file utils.cpp
 * @author Qasim Khawaja (khawaja@ualberta.ca)
 * @brief Utility functions for the PSRS algorithm.
 * @version 0.1
 * @date 2022-09-11
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "utils.hpp"

bool comparePair(const std::pair<long, int>& a, const std::pair<long, int>& b)
{
    return a.first > b.first;
}
int compare(const void* a, const void* b)
{
    return (*(long*)a - *(long*)b);
}
