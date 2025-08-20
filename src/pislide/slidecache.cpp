
#include "slidecache.h"
#include "constants.h"

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

        // Convert to a texture.
        auto beginTime = std::chrono::high_resolution_clock::now();
        std::shared_ptr<Image> image = loadedImage.image ? loadedImage.image : mBrokenImage;
        Texture texture = LoadTextureFromImage(*image);
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto prepTime = endTime - beginTime;

        // Add to our cache.
        auto slide = std::make_shared<Slide>(loadedImage.photo, texture,
                loadedImage.loadTime, prepTime, !loadedImage.image);
        slide->computeIdealSize(mScreenWidth, mScreenHeight);
        mCache[loadedImage.photo.id] = slide;
    }
}

void SlideCache::shrinkCache() {
    while (mCache.size() >= SLIDE_CACHE_SIZE) {
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
    return SLIDE_CACHE_SIZE;
}
