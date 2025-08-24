
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

    Config const &mConfig;
    bool mDeleteMessages = false;
    bool mDeleteImages = false;
    double mPreviousFetch = 0;

    // Fetch the images. Runs in a different thread.
    static Response fetchImagesInThread(Request const &request);

public:
    TwilioFetcher(Config const &config);

    /**
     * Whether to delete Twilio messages after having fetched them.
     * Defaults to false.
     */
    void setDeleteMessages(bool deleteMessages) {
        mDeleteMessages = deleteMessages;
    }

    /**
     * Whether to delete Twilio images after having fetched them.
     * Defaults to false.
     */
    void setDeleteImages(bool deleteImages) {
        mDeleteImages = deleteImages;
    }

    /**
     * Request an asynchronous fetch of Twilio images. It's safe to call this
     * multiple times even if an ongoing fetch is happening.
     */
    void initiateFetch();

    /**
     * Same as initiateFetch(), but throttled to be no more frequent than
     * every "throttleSeconds".
     */
    void initiateFetch(double throttleSeconds);

    /**
     * Return the images that have been fetched, if any.
     */
    std::vector<std::shared_ptr<TwilioImage>> get();
};
