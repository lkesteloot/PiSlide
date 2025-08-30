
#include <iostream>

#include <spdlog/spdlog.h>

#include "twiliofetcher.h"
#include "util.h"

TwilioFetcher::TwilioFetcher(Config const &config)
    : mExecutor(1, []<typename T0>(T0 && PH1) { return fetchImagesInThread(std::forward<T0>(PH1)); }),
    mConfig(config) {}

void TwilioFetcher::initiateFetch() {
    if (!mConfig.twilioSid.empty() && !mConfig.twilioToken.empty()) {
        // We don't want these to queue up, one is enough.
        spdlog::info("TwilioFetcher: Initiating fetch");
        mExecutor.clearRequestQueue();
        mExecutor.ask(Request {
            .deleteMessages = mDeleteMessages,
            .deleteImages = mDeleteImages,
            .config = mConfig,
        });
    }
}

void TwilioFetcher::initiateFetch(double throttleSeconds) {
    double now = nowArbitrary();
    if (now - mPreviousFetch >= throttleSeconds) {
        mPreviousFetch = now;
        initiateFetch();
    }
}

std::vector<std::shared_ptr<TwilioImage>> TwilioFetcher::get() {
    std::optional<Response> response = mExecutor.get();
    return response.has_value()
        ? response->images
        : std::vector<std::shared_ptr<TwilioImage>>();
}

TwilioFetcher::Response TwilioFetcher::fetchImagesInThread(Request const &request) {
    spdlog::info("TwilioFetcher: Fetch in thread");
    return Response {
        .images = downloadTwilioImages(
                request.deleteMessages, request.deleteImages, request.config),
    };
}
