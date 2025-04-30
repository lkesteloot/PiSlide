
#include "slidecache.h"

constexpr int MAX_CACHE_SIZE = 4;

std::shared_ptr<Slide> SlideCache::get(Photo const &photo) {
    // Before doing anything, see if the loader has anything for us.
    checkImageLoader();

    if (mCache.contains(photo.id)) {
        return mCache.at(photo.id);
    }

    // Cap size of cache. Do this before we load the next slide.
    shrinkCache();

    // Load the photo.
    mImageLoader.requestImage(photo);

    return std::shared_ptr<Slide>();
}

void SlideCache::checkImageLoader() {
    for (auto &[photo, image] : mImageLoader.getImages()) {
        // Make sure the cache has space.
        shrinkCache();

        // The slide might be None if we couldn't load it. This causes problems
        // throughout the rest of the program, and we can't easily purge it
        // from the cache, so we make a fake slide.
        // TODO support this.
        //if not slide:
            //slide = BrokenSlide()

        Texture texture = LoadTextureFromImage(*image);
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
        auto slide = std::make_shared<Slide>(photo, texture);
        slide->computeIdealSize(mScreenWidth, mScreenHeight);
        mCache[photo.id] = slide;

        /*
        if slide.is_broken:
            LOGGER.error("Couldn't load image")
        else:
            LOGGER.debug("Reading image took %.1f seconds (%d frames)." % (slide.load_time, slide.frame_count))
            */
    }
}

void SlideCache::shrinkCache() {
    while (mCache.size() >= MAX_CACHE_SIZE) {
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
