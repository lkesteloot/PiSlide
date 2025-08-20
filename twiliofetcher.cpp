
#include "twiliofetcher.h"

TwilioFetcher::TwilioFetcher()
    : mExecutor(1, []<typename T0>(T0 && PH1) { return fetchImagesInThread(std::forward<T0>(PH1)); }) {}

void TwilioFetcher::fetch(std::filesystem::path const &imageDir,
        bool deleteMessages, bool deleteImages, Config const &config) {

    // We don't want these to queue up, one is enough.
    mExecutor.clearRequestQueue();
    mExecutor.ask(Request {
        .imageDir = imageDir,
        .deleteMessages = deleteMessages,
        .deleteImages = deleteImages,
        .config = config,
    });
}

std::vector<std::shared_ptr<TwilioImage>> TwilioFetcher::get() {
    std::optional<Response> response = mExecutor.get();
    return response.has_value()
        ? response->images
        : std::vector<std::shared_ptr<TwilioImage>>();
}

TwilioFetcher::Response TwilioFetcher::fetchImagesInThread(Request const &request) {
    return Response {
        .images = downloadTwilioImages(
                request.imageDir, request.deleteMessages, request.deleteImages, request.config),
    };
}
