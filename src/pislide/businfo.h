
#pragma once

#include <ctime>
#include <vector>

#include "executor.h"
#include "config.h"

/**
 * Asynchronously fetches bus information.
 */
class BusInfo final {
    // Request sent to the executor's thread.
    struct Request {
        Config const &config;
    };

    // Response from the executor's thread.
    struct Response {
        std::vector<time_t> times;
    };

    // Executor to fetch the bus data in another thread.
    Executor<Request,Response> mExecutor;

    // When we most recently fetched data.
    time_t mMostRecentFetch;

    // The most recent response.
    Response mMostRecentResponse;

    // Fetch the bus data. Runs in a different thread.
    static Response fetchBusDataInThread(Request const &request);

public:
    BusInfo();

    // Can't copy.
    BusInfo(const BusInfo &) = delete;
    BusInfo &operator=(const BusInfo &) = delete;

    /**
     * Return a sorted vector of epoch times that the bus is expected to
     * show up. If no bus information has been configured, or if anything
     * goes wrong fetching the information, an empty vector is returned.
     */
    std::vector<time_t> getTimes(Config const &config);
};
