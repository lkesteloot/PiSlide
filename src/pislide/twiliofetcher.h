
#pragma once

#include <vector>
#include <memory>
#include <filesystem>

#include "executor.h"
#include "twilio.h"
#include "config.h"

/**
 * Asynchronously fetches Twilio images.
 */
class TwilioFetcher final {
    // Request sent to the executor's thread.
    struct Request {
        std::filesystem::path imageDir;
        bool deleteMessages;
        bool deleteImages;
        Config const &config;
    };

    // Response from the executor's thread.
    struct Response {
        std::vector<std::shared_ptr<TwilioImage>> images;
    };

    // Executor to fetch images in another thread.
    Executor<Request,Response> mExecutor;

    // Fetch the images. Runs in a different thread.
    static Response fetchImagesInThread(Request const &request);

public:
    TwilioFetcher();

    /**
     * Request an asynchronous fetch of Twilio images. It's safe to call this
     * multiple times even if an ongoing fetch is happening.
     */
    void fetch(std::filesystem::path const &imageDir,
            bool deleteMessages, bool deleteImages,
            Config const &config);

    /**
     * Return the images that have been fetched, if any.
     */
    std::vector<std::shared_ptr<TwilioImage>> get();
};
