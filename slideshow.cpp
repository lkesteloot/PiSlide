
#include <cmath>

#include "raylib.h"

#include "slideshow.h"
#include "util.h"
#include "constants.h"

bool Slideshow::loopRunning() const {
    return !WindowShouldClose();
}

void Slideshow::move() {
    // Amount of time since last frame.
    double frameTime = now();
    double deltaTime = mPreviousFrameTime == 0 ? 0 : frameTime - mPreviousFrameTime;
    mPreviousFrameTime = frameTime;

    /*
    // Auto-disable paused after a while.
    if self.paused && self.pause_start_time is not None && time.time() - self.pause_start_time >= MAX_PAUSE_SECONDS:
        self.paused = False
        self.pause_start_time = None

    // Auto-disable bus after a while.
    if self.showing_bus && self.bus_start_time is not None && time.time() - self.bus_start_time >= MAX_BUS_SECONDS:
        self.set_show_bus(False)

    // Auto-disable Twilio after a while.
    if self.fetching_twilio && self.twilio_start_time is not None && time.time() - self.twilio_start_time >= MAX_TWILIO_SECONDS:
        self.set_fetch_twilio(False)
    */

    // Advance time if not paused.
    if (!mPaused) {
        mTime += deltaTime;
    }

    auto cs = getCurrentSlides();
    if (cs.currentSlide && !cs.currentSlide->isBroken()) {
        cs.currentSlide->move(mPaused, false, cs.currentTimeOffset);
    }
    if (cs.nextSlide && !cs.nextSlide->isBroken()) {
        cs.nextSlide->move(mPaused, false, cs.nextTimeOffset);
    }
}

void Slideshow::draw() {
    auto cs = getCurrentSlides();

    BeginDrawing();
    ClearBackground(BLACK);

    // Check configured here because there's a chance that we'll
    // get a loaded slide from the loader between the move and
    // the draw, meaning that the draw will happen before it's
    // been positioned. Skip drawing in case the bad positioning
    // causes problems (despite zero alpha).
    if (cs.currentSlide && !cs.currentSlide->isBroken() && cs.currentSlide->configured()) {
        cs.currentSlide->draw(mScreenWidth, mScreenHeight);
    }
    if (cs.nextSlide && !cs.nextSlide->isBroken() && cs.nextSlide->configured()) {
        cs.nextSlide->draw(mScreenWidth, mScreenHeight);
    }

    // Any slide we didn't draw we should mark as not configured so that
    // next time it's drawn it can jump immediately to its initial position.
    /*
    for slide in self.slide_cache.get_slides():
        if slide is not cs.currentSlide && slide is not cs.nextSlide && not slide.isBroken():
            slide.turn_off()
            */

    // Upper-left:
    /*
    if self.prompting_email:
        self.draw_email_prompt()
    elif self.show_debug:
        self.draw_debug()
    elif self.showing_bus:
        self.draw_bus()

    // Upper-right:
    self.draw_time()
    self.draw_sonos()
    */
    DrawFPS(10, 10);
    EndDrawing();
}

Slideshow::CurrentSlides Slideshow::getCurrentSlides() {
    int photoIndex = getCurrentPhotoIndex();
    CurrentSlides currentSlides {
        .index = photoIndex,
        .currentSlide = std::shared_ptr<Slide>(),
        .currentTimeOffset = 0.0,
        .nextSlide = std::shared_ptr<Slide>(),
        .nextTimeOffset = 0.0,
    };

    if (mDbPhotos.empty()) {
        return currentSlides;
    }

    currentSlides.currentSlide = mSlideCache.get(photoByIndex(photoIndex));
    if (currentSlides.currentSlide && !currentSlides.currentSlide->swapZoom().has_value()) {
        currentSlides.currentSlide->setSwapZoom(modulo(photoIndex, 2) == 0);
    }
    currentSlides.currentTimeOffset = mTime - photoIndex*SLIDE_DISPLAY_S;

    if (currentSlides.currentTimeOffset >= SLIDE_DISPLAY_S - SLIDE_TRANSITION_S) {
        currentSlides.nextSlide = mSlideCache.get(photoByIndex(photoIndex + 1));
        if (currentSlides.nextSlide && !currentSlides.nextSlide->swapZoom().has_value()) {
            currentSlides.nextSlide->setSwapZoom(modulo(photoIndex + 1, 2) == 0);
        }
        currentSlides.nextTimeOffset = currentSlides.currentTimeOffset - SLIDE_DISPLAY_S;
    }

    return currentSlides;
}

int Slideshow::getCurrentPhotoIndex() const {
    return static_cast<int>(std::floor(mTime/SLIDE_DISPLAY_S));
}

Photo Slideshow::photoByIndex(int index) const {
    return mDbPhotos.at(modulo(index, mDbPhotos.size()));
}
