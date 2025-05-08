
#pragma once

#include <ctime>
#include <vector>

#include "config.h"

/**
 * Return a sorted vector of epoch times that the bus is expected to
 * show up. If no bus information has been configured, or if anything
 * goes wrong fetching the information, an empty vector is returned.
 */
std::vector<time_t> nextBuses(Config const &config);

