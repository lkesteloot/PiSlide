
#include "slidecache.h"

constexpr int CACHE_SIZE = 4;

std::shared_ptr<Slide> SlideCache::get(Photo const &photo, bool fetch) {
    // Before doing anything, see if the loader has anything for us.
    checkImageLoader();

    if (mCache.contains(photo.id)) {
        return mCache.at(photo.id);
    }

    if (fetch) {
        // Cap size of cache. Do this before we load the next slide.
        shrinkCache();

        // Load the photo.
        mImageLoader.requestImage(photo);
    }

    // Empty pointer to indicate that we don't have it.
    return std::shared_ptr<Slide>();
}

void SlideCache::checkImageLoader() {
    for (auto &loadedImage : mImageLoader.getLoadedImages()) {
        // Make sure the cache has space.
        shrinkCache();

        // The slide might be None if we couldn't load it. This causes problems
        // throughout the rest of the program, and we can't easily purge it
        // from the cache, so we make a fake slide.
        // TODO support this.
        //if not slide:
            //slide = BrokenSlide()

        // TODO test this, factor out, move into loading thread.
        {
            Image *imagePtr = loadedImage.image.get();
            constexpr int BORDER = 4;
            int width = imagePtr->width;
            int height = imagePtr->height;
            // Top.
            for (int y = 0; y < BORDER; y++) {
                for (int x = 0; x < width; x++) {
                    ImageDrawPixel(imagePtr, x, y, BLANK);
                }
            }
            // Bottom.
            for (int y = height - BORDER; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    ImageDrawPixel(imagePtr, x, y, BLANK);
                }
            }
            for (int y = 0; y < height; y++) {
                // Left.
                for (int x = 0; x < BORDER; x++) {
                    ImageDrawPixel(imagePtr, x, y, BLANK);
                }
                // Right.
                for (int x = width - BORDER; x < width; x++) {
                    ImageDrawPixel(imagePtr, x, y, BLANK);
                }
            }
        }

        // Prepare texture.
        auto beginTime = std::chrono::high_resolution_clock::now();
        Texture texture = LoadTextureFromImage(*loadedImage.image);
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto prepTime = endTime - beginTime;

        // Add to our cache.
        auto slide = std::make_shared<Slide>(loadedImage.photo, texture, loadedImage.loadTime, prepTime);
        slide->computeIdealSize(mScreenWidth, mScreenHeight);
        mCache[loadedImage.photo.id] = slide;

        /*
        if slide.is_broken:
            LOGGER.error("Couldn't load image")
        else:
            LOGGER.debug("Reading image took %.1f seconds (%d frames)." % (slide.load_time, slide.frame_count))
            */
    }
}

void SlideCache::shrinkCache() {
    while (mCache.size() >= CACHE_SIZE) {
        purgeOldest();
    }
}

void SlideCache::purgeOldest() {
    int32_t oldestPhotoId = -1;
    double oldestTime = 0;

    for (auto &[photoId, slide] : mCache) {
        // TODO Seems like we either always or never purge failed
        // slides, but really we shouldn't put them in this cache
        // at all, just keep a separate set of failed photo IDs.
        double lastUsed = slide ? slide->lastUsed() : 0;
        if (oldestPhotoId == -1 || lastUsed < oldestTime) {
            oldestPhotoId = photoId;
            oldestTime = lastUsed;
        }
    }

    if (oldestPhotoId != -1) {
        mCache.erase(oldestPhotoId);
    }
}

void SlideCache::resetUnused(std::shared_ptr<Slide> currentSlide,
        std::shared_ptr<Slide> nextSlide) {

    for (auto &[photoId, slide] : mCache) {
        if (slide != currentSlide && slide != nextSlide && !slide->isBroken()) {
            slide->reset();
        }
    }
}

int SlideCache::cacheSize() const {
    return CACHE_SIZE;
}
