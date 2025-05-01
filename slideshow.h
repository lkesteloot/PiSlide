
#pragma once

#include <vector>

#include "database.h"
#include "slide.h"
#include "slidecache.h"
#include "textwriter.h"

/**
 * Runs the whole slideshow (animates and draws slides, handles user input, ...).
 */
class Slideshow final {
    std::vector<Photo> const &mDbPhotos;
    int mScreenWidth;
    int mScreenHeight;
    Database const &mDatabase;
    SlideCache mSlideCache;
    TextWriter mTextWriter;
    double mPreviousFrameTime = 0;
    double mTime = 0;
    bool mPaused = false;
    double mPauseStartTime = 0;
    bool mDebug = false;
    bool mQuit = false;

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

    // Draw various things.
    void drawTime();
    void drawDebug();

    // Control the slideshow.
    void jumpRelative(int deltaPhoto);
    void togglePause();
    void toggleDebug();

    // Modify the slide.
    void rotatePhoto(int degrees);
    void ratePhoto(int rating);

public:
    Slideshow(std::vector<Photo> const &dbPhotos,
            int screenWidth,
            int screenHeight,
            Database const &database)
        : mDbPhotos(dbPhotos),
        mScreenWidth(screenWidth),
        mScreenHeight(screenHeight),
        mDatabase(database),
        mSlideCache(screenWidth, screenHeight) {

        // We'll handle this.
        SetExitKey(0);
    }

    // Can't copy.
    Slideshow(const Slideshow &) = delete;
    Slideshow &operator=(const Slideshow &) = delete;

    bool loopRunning() const;
    void prefetch();
    void move();
    void draw(Texture const &starTexture);
    void handleKeyboard();
};

