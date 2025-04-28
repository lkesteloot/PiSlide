
#include <cmath>
#include <ctime>

#include "raylib.h"

#include "slideshow.h"
#include "util.h"
#include "constants.h"

bool Slideshow::loopRunning() const {
    return !WindowShouldClose() && !mQuit;
}

void Slideshow::move() {
    // Amount of time since last frame.
    double frameTime = now();
    double deltaTime = mPreviousFrameTime == 0 ? 0 : frameTime - mPreviousFrameTime;
    mPreviousFrameTime = frameTime;

    // Auto-disable paused after a while.
    if (mPaused && mPauseStartTime != 0 && now() - mPauseStartTime >= MAX_PAUSE_SECONDS) {
        mPaused = false;
        mPauseStartTime = 0;
    }

    /*
    // Auto-disable bus after a while.
    if self.showing_bus && self.bus_start_time is not None && time.time() - self.bus_start_time >= MAX_BUS_SECONDS:
        self.set_show_bus(false)

    // Auto-disable Twilio after a while.
    if self.fetching_twilio && self.twilio_start_time is not None && time.time() - self.twilio_start_time >= MAX_TWILIO_SECONDS:
        self.set_fetch_twilio(false)
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
        cs.currentSlide->draw(mTextWriter, mScreenWidth, mScreenHeight);
    }
    if (cs.nextSlide && !cs.nextSlide->isBroken() && cs.nextSlide->configured()) {
        cs.nextSlide->draw(mTextWriter, mScreenWidth, mScreenHeight);
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
        } else if (self.show_debug:
        self.draw_debug()
        } else if (self.showing_bus:
        self.draw_bus()

    // Upper-right:
    self.draw_time()
    self.draw_sonos()
    */
    drawTime();
    DrawFPS(DISPLAY_MARGIN, DISPLAY_MARGIN);
    EndDrawing();
}

Slideshow::CurrentSlides Slideshow::getCurrentSlides() {
    int photoIndex = getCurrentPhotoIndex();
    CurrentSlides cs {
        .index = photoIndex,
        .currentSlide = std::shared_ptr<Slide>(),
        .currentTimeOffset = 0.0,
        .nextSlide = std::shared_ptr<Slide>(),
        .nextTimeOffset = 0.0,
    };

    if (mDbPhotos.empty()) {
        return cs;
    }

    cs.currentSlide = mSlideCache.get(photoByIndex(photoIndex));
    if (cs.currentSlide && !cs.currentSlide->swapZoom().has_value()) {
        cs.currentSlide->setSwapZoom(modulo(photoIndex, 2) == 0);
    }
    cs.currentTimeOffset = mTime - photoIndex*SLIDE_DISPLAY_S;

    if (cs.currentTimeOffset >= SLIDE_DISPLAY_S - SLIDE_TRANSITION_S) {
        cs.nextSlide = mSlideCache.get(photoByIndex(photoIndex + 1));
        if (cs.nextSlide && !cs.nextSlide->swapZoom().has_value()) {
            cs.nextSlide->setSwapZoom(modulo(photoIndex + 1, 2) == 0);
        }
        cs.nextTimeOffset = cs.currentTimeOffset - SLIDE_DISPLAY_S;
    }

    return cs;
}

int Slideshow::getCurrentPhotoIndex() const {
    return static_cast<int>(std::floor(mTime/SLIDE_DISPLAY_S));
}

Photo Slideshow::photoByIndex(int index) const {
    return mDbPhotos.at(modulo(index, mDbPhotos.size()));
}

void Slideshow::handleKeyboard() {
    // Unicode code point.
    int ch = GetCharPressed();
    if (ch != KEY_NULL) {
        std::cout << "Got char " << ch << std::endl; // TODO remove
        /*
        if self.prompting_email:
            if ch == 27: # ESC
                self.prompting_email = false
                self.paused = false
                self.mPauseStartTime = None
                } else if (ch == 8 or ch == 263:
                if len(self.email_address) > 0:
                    self.email_address = self.email_address[:-1]
                    self.update_suggested_emails()
                    } else if (ch == 9:
                if len(self.suggested_emails) == 1:
                    self.complete_email(0)
                    } else if (ch == 10 or ch == 13:
                self.email_address = self.email_address.strip()
                if self.email_address:
                    # Submit email address.
                    success = self.email_photo(self.email_address)
                else:
                    success = true
                if success:
                    self.prompting_email = false
                    self.paused = false
                    self.mPauseStartTime = None
                else:
                    self.email_error = true
                    } else if (ch >= 32 and ch < 128:
                self.email_address += chr(ch)
                self.update_suggested_emails()
                } else if (ch >= FIRST_FUNCTION_KEY and ch < FIRST_FUNCTION_KEY + MAX_SUGGESTED_EMAILS:
                index = ch - FIRST_FUNCTION_KEY
                if index < len(self.suggested_emails):
                    self.complete_email(index)
                    } else if (ch == FIRST_FUNCTION_KEY + 12 - 1:
                self.email_from = (self.email_from + 1) % len(config.EMAIL_FROM)
            else:
                # Ignore ch.
                LOGGER.info("Unknown key during email entry: " + str(ch))
            return true
        else:
            return false
            */

        if (ch == 'Q') {
            mQuit = true;
        } else if (ch == 'r') {
            // slideshow.rotate_clockwise()
        } else if (ch == 'l') {
            // slideshow.rotate_counterclockwise()
        } else if (ch == ' ') {
            togglePause();
        } else if (ch == 'D') {
            // slideshow.toggle_debug()
        } else if (ch == 'm') {
            // slideshow.mute()
        } else if (ch == 's') {
            // slideshow.stop_music()
        } else if (ch == 'e') {
            // slideshow.prompt_email()
        } else if (ch == 'b') {
            // slideshow.toggle_bus()
        } else if (ch == 'T') {
            // slideshow.toggle_twilio()
        } else if (ch >= '1' and ch <= '5') {
            // slideshow.rate_photo(ch - '1' + 1)
        } else {
            std::cout << "Got unknown char " << ch << std::endl; // TODO remove
        }
    }

    // Non-character keys.
    int key = GetKeyPressed();
    if (key != 0) {
        std::cout << "Got key " << key << std::endl; // TODO remove
        if (key == KEY_LEFT) {
            jumpRelative(-1);
        } else if (key == KEY_RIGHT) {
            jumpRelative(1);
        } else if (key >= KEY_F1 and key <= KEY_F12) {
            // slideshow.play_radio_station(key - KEY_F1);
        } else {
            std::cout << "Got unknown key " << key << std::endl; // TODO remove
        }
    }
}

void Slideshow::jumpRelative(int deltaSlide) {
    auto cs = getCurrentSlides();
    mTime = std::max(0.0, mTime + deltaSlide*SLIDE_DISPLAY_S - cs.currentTimeOffset);
}

void Slideshow::togglePause() {
    mPaused = !mPaused;
    mPauseStartTime = mPaused ? now() : 0;
}

void Slideshow::drawTime() {
    // All-C API because the C++ stuff is really bad at this.
    std::time_t now = std::time(nullptr);
    std::tm localTime = *std::localtime(&now);

    char buffer[6];  // Enough for "h:mm\0" or "hh:mm\0"
    std::strftime(buffer, sizeof(buffer), "%-I:%M", &localTime);

    mTextWriter.write(buffer, Vector2 { mScreenWidth - DISPLAY_MARGIN, DISPLAY_MARGIN },
            64, WHITE, TextWriter::Alignment::END, TextWriter::Alignment::START);
}
