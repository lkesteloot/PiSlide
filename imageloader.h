
#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <set>

#include "raylib.h"

#include "model.h"
#include "executor.h"
#include "util.h"

/**
 * Information about an image that was loaded.
 */
struct LoadedImage {
    Photo photo;
    // If the pointer is null, the image failed to load.
    std::shared_ptr<Image> image;
    Timing loadTime;
};

/**
 * Asynchronously loads photos, creating Image objects.
 */
class ImageLoader final {
    // Request sent to the executor's thread.
    struct Request {
        Photo photo;
    };

    // Response from the executor's thread.
    struct Response {
        Photo photo;
        std::shared_ptr<Image> image;
        Timing loadTime;
    };

    // Set of photo IDs that are either in the request queue or
    // currently being loaded.
    std::set<int32_t> mAlreadyRequestedIds;

    // Executor to load the photos in another thread.
    Executor<Request,Response> mExecutor;

    // Load the photo. Runs in a different thread.
    static Response loadPhotoInThread(Request const &request);

public:
    ImageLoader();

    /**
     * Request an asynchronous load of the photo. It's safe to call this
     * multiple times with the same photo before or while the photo is loading.
     */
    void requestImage(Photo const &photo);

    /**
     * Fetch the images that have been loaded. The shared pointer is
     * empty if the image failed to load.
     */
    std::vector<LoadedImage> getLoadedImages();
};
