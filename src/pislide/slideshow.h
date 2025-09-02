
#pragma once

#include <vector>
#include <optional>

#include "qrcodegen.hpp"

#include "config.h"
#include "database.h"
#include "slide.h"
#include "slidecache.h"
#include "textwriter.h"
#include "businfo.h"

#include <spdlog/sinks/ringbuffer_sink.h>

/**
 * Runs the whole slideshow (animates and draws slides, handles user input, ...).
 */
class Slideshow final {
    std::vector<Photo> &mDbPhotos;
    int mScreenWidth;
    int mScreenHeight;
    Config const &mConfig;
    Database const &mDatabase;
    TextWriter mTextWriter;
    SlideCache mSlideCache;
    std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> mLogRingBufferSink;
    BusInfo mBusInfo;
    double mPreviousFrameTime = 0;
    double mTime = 0;
    bool mPaused = false;
    double mPauseStartTime = 0;
    bool mShowingBus = false;
    double mBusStartTime = 0;
    bool mDebug = false;
    bool mParty = false;
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
    void drawTime(Color color);
    void drawBus(Color color);
    void drawDebug();

    // Control the slideshow.
    void jumpRelative(int deltaPhoto);
    void togglePause();
    void toggleBus();
    void toggleDebug();
    void toggleParty();

    // Modify the slide.
    void rotatePhoto(int degrees);
    void ratePhoto(int rating);

    // Make the image that will be used if an image file fails to load.
    static std::shared_ptr<Image> makeBrokenImage(TextWriter &textWriter);

public:
    Slideshow(std::vector<Photo> &dbPhotos,
            int screenWidth,
            int screenHeight,
            Config const &config,
            Database const &database,
            std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ringBufferSink)
        : mDbPhotos(dbPhotos),
        mScreenWidth(screenWidth),
        mScreenHeight(screenHeight),
        mConfig(config),
        mDatabase(database),
        mSlideCache(screenWidth, screenHeight, makeBrokenImage(mTextWriter)),
        mLogRingBufferSink(ringBufferSink) {

        // We'll handle this.
        SetExitKey(0);
    }

    // Can't copy.
    Slideshow(const Slideshow &) = delete;
    Slideshow &operator=(const Slideshow &) = delete;

    bool loopRunning() const;
    void prefetch();
    void move();
    void draw(Texture const &starTexture, std::optional<qrcodegen::QrCode> const &qrCode);
    void handleKeyboard();
    void insertPhoto(Photo const &photo);
    bool isParty() const { return mParty; }
};

