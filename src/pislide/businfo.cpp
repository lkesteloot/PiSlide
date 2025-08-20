
#include "businfo.h"
#include "util.h"
#include "constants.h"
#include "webservices.h"

BusInfo::BusInfo()
    : mExecutor(1, []<typename T0>(T0 && PH1) { return fetchBusDataInThread(std::forward<T0>(PH1)); }),
        mMostRecentFetch(0) {}

std::vector<time_t> BusInfo::getTimes(Config const &config) {
    // See how long it's been since we fetched the info.
    time_t now = nowEpoch();
    time_t elapsed = now - mMostRecentFetch;

    // If too long, fetch it.
    if (elapsed > BUS_INFO_FETCH_S) {
        mMostRecentFetch = now;
        mExecutor.ask(Request {
            .config = config,
        });
    }

    // Drain the reply queue.
    std::optional<Response> response = mExecutor.getMostRecent();
    if (response) {
        mMostRecentResponse = *response;
    }

    // Reply with the latest estimates.
    return mMostRecentResponse.times;
}

BusInfo::Response BusInfo::fetchBusDataInThread(Request const &request) {
    return Response {
        .times = nextBuses(request.config),
    };
}
