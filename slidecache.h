
#pragma once

#include <cstdint>
#include <vector>
#include <utility>
#include <memory>
#include <map>
#include <set>

#include "model.h"
#include "slide.h"
#include "imageloader.h"

/**
 * Cache from photo to slide. Loads slides on demand when a photo is
 * asked for and isn't in the cache.
 */
class SlideCache {
    // Map from photo ID to Slide. The pointer is empty if the image
    // failed to load.
    std::map<int32_t,std::shared_ptr<Slide>> mCache;

    // Loads images asynchronously.
    ImageLoader mImageLoader;

    // Size of window;
    int mScreenWidth;
    int mScreenHeight;

    // Drain the return queue of the loader.
    void checkImageLoader();

    // Leave at least one place in the cache.
    void shrinkCache();

    // Purge one slide that was used least recently.
    void purgeOldest();

public:
    SlideCache(int screenWidth, int screenHeight)
        : mScreenWidth(screenWidth), mScreenHeight(screenHeight) {}
    virtual ~SlideCache() {}

    /**
     * Return immediately with a Slide object if we've loaded this photo,
     * or return an empty pointer and start loading the photo asynchronously.
     */
    std::shared_ptr<Slide> get(Photo const &photo);
};
