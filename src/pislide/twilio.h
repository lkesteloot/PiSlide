
#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <memory>

#include "config.h"

/**
 * Information about downloaded Twilio image.
 */
struct TwilioImage final {
    std::filesystem::path pathname;
    std::string contentType;
    std::string sourcePhoneNumber;
    std::string dateSent;
};

/**
 * Connects to Twilio, downloads available messages, downloads their
 * images to files on disk, and optionally deletes the images and/or messages.
 *
 * imageDir: directory where to save the image
 *
 * Returns the list of downloaded images. If something goes wrong, returns
 * an empty vector.
 */
std::vector<std::shared_ptr<TwilioImage>> downloadTwilioImages(
        std::filesystem::path const &imageDir,
        bool deleteMessages, bool deleteImages,
        Config const &config);

