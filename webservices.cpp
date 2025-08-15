
#include <string>
#include <iostream>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include "webservices.h"

using namespace std::literals::string_literals;

std::vector<long> nextBuses(Config const &config) {
    std::vector<long> times;

    // See if we're configured at all.
    if (config.bus511orgToken.empty() || config.bus511orgAgency.empty() || config.bus511orgStopCode.empty()) {
        std::cerr << "511.org is not configured\n";
        return times;
    }

    // Fetch the information.
    // TODO use cpr::Parameters
    auto url = "https://api.511.org/transit/StopMonitoring?api_key="s + config.bus511orgToken +
        "&agency=" + config.bus511orgAgency +
        "&stopCode=" + config.bus511orgStopCode +
        "&format=json";

    cpr::Response r = cpr::Get(cpr::Url{url});
    if (r.status_code != 200) {
        std::cerr << "Got status code " << r.status_code << " fetching 511.org information.\n";
        return times;
    }

    // Parse the information.
    // TODO fix variable names.
    auto x = nlohmann::json::parse(r.text);
    for (auto const &y : x["ServiceDelivery"]["StopMonitoringDelivery"]["MonitoredStopVisit"]) {
        std::string timestamp = y["MonitoredVehicleJourney"]["MonitoredCall"]["ExpectedArrivalTime"];
        if (!timestamp.empty()) {
            struct std::tm tm = {};
            char *p = strptime(timestamp.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm);
            if (p == nullptr || *p != '\0') {
                std::cerr << "Unable to parse 511.org timestamp " << timestamp << '\n';
            } else {
                times.emplace_back(timegm(&tm));
            }
        }
    }

    return times;
}
