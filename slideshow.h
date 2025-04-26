
#pragma once

#include <vector>

#include "database.h"
#include "slide.h"
#include "slidecache.h"

class Slideshow {
    std::vector<Photo> const &mDbPhotos;
    int mScreenWidth;
    int mScreenHeight;
    Database const &mDatabase;
    SlideCache mSlideCache;
    double mPreviousFrameTime = 0;
    double mTime = 0;
    bool mPaused = false;

    /**
     * Information about the slides we're showing now.
     */
    struct CurrentSlides {
        // Current slide index.
        int index;
        // Current slide if it loaded.
        std::shared_ptr<Slide> currentSlide;
        // Time into current slide (always valid).
        double currentTimeOffset;
        // Next slide if it loaded and we're showing it.
        std::shared_ptr<Slide> nextSlide;
        // Time into next slide (negative if we're showing it).
        double nextTimeOffset;
    };

    // Get information about the current slides.
    CurrentSlides getCurrentSlides();

    // Compute the current index based on the time.
    int getCurrentPhotoIndex() const;

    // Get the photo by its index, where index can go on indefinitely.
    Photo photoByIndex(int index) const;

public:
    Slideshow(std::vector<Photo> const &dbPhotos,
            int screenWidth,
            int screenHeight,
            Database const &database)
        : mDbPhotos(dbPhotos),
        mScreenWidth(screenWidth),
        mScreenHeight(screenHeight),
        mDatabase(database),
        mSlideCache(screenWidth, screenHeight) {}
    virtual ~Slideshow() {}

    // Can't copy.
    Slideshow(const Slideshow &) = delete;
    Slideshow &operator=(const Slideshow &) = delete;

    bool loopRunning() const;
    void move();
    void draw();
};

