
#include <iostream>

#include "twiliofetcher.h"
#include "util.h"

TwilioFetcher::TwilioFetcher(Config const &config)
    : mExecutor(1, []<typename T0>(T0 && PH1) { return fetchImagesInThread(std::forward<T0>(PH1)); }),
    mConfig(config) {}

void TwilioFetcher::initiateFetch() {
    // We don't want these to queue up, one is enough.
    std::cout << "TwilioFetcher: Initiating fetch\n";
    mExecutor.clearRequestQueue();
    mExecutor.ask(Request {
        .deleteMessages = mDeleteMessages,
        .deleteImages = mDeleteImages,
        .config = mConfig,
    });
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
    std::cout << "TwilioFetcher: Fetch in thread\n";
    return Response {
        .images = downloadTwilioImages(
                request.deleteMessages, request.deleteImages, request.config),
    };
}
