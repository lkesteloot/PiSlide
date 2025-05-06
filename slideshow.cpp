
#include <cmath>
#include <sstream>

#include "raylib.h"

#include "slideshow.h"
#include "util.h"
#include "constants.h"

bool Slideshow::loopRunning() const {
    return !WindowShouldClose() && !mQuit;
}

void Slideshow::prefetch() {
    int n = mSlideCache.cacheSize()/2 + 1;
    int photoIndex = getCurrentPhotoIndex();
    for (int i = 0; i < n; i++) {
        auto slide = mSlideCache.get(photoByIndex(photoIndex + i));
        if (slide) {
            // Consider the prefetched slides touched because we always prefer them
            // to the oldest slides.
            // (TODO: why?)
            slide->touch();
        }
    }
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
    if (cs.currentSlide) {
        cs.currentSlide->move(mPaused, false, cs.currentTimeOffset);
    }
    if (cs.nextSlide) {
        cs.nextSlide->move(mPaused, false, cs.nextTimeOffset);
    }
}

void Slideshow::draw(Texture const &starTexture) {
    auto cs = getCurrentSlides();

    BeginDrawing();
    ClearBackground(BLACK);

    float fade = mDebug ? 0.3f : 1.0f;

    // Check configured here because there's a chance that we'll
    // get a loaded slide from the loader between the move and
    // the draw, meaning that the draw will happen before it's
    // been positioned. Skip drawing in case the bad positioning
    // causes problems (despite zero alpha).
    if (cs.currentSlide && cs.currentSlide->configured()) {
        cs.currentSlide->draw(mTextWriter, starTexture, mScreenWidth, mScreenHeight, fade);
    }
    if (cs.nextSlide && cs.nextSlide->configured()) {
        cs.nextSlide->draw(mTextWriter, starTexture, mScreenWidth, mScreenHeight, fade);
    }

    // Reset any slide we didn't draw so that next time it's drawn it can jump
    // immediately to its initial position.
    mSlideCache.resetUnused(cs.currentSlide, cs.nextSlide);

    // Upper-left:
    if (mDebug) {
        drawDebug();
    }

    // Upper-right:
    drawTime(Fade(WHITE, fade));

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
        std::cout << "Got char " << ch << '\n'; // TODO remove
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
            rotatePhoto(-90);
        } else if (ch == 'l') {
            rotatePhoto(90);
        } else if (ch == ' ') {
            togglePause();
        } else if (ch == 'D') {
            toggleDebug();
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
            ratePhoto(ch - '1' + 1);
        } else {
            std::cout << "Got unknown char " << ch << '\n'; // TODO remove
        }
    }

    // Non-character keys.
    int key = GetKeyPressed();
    if (key != 0) {
        std::cout << "Got key " << key << '\n'; // TODO remove
        if (key == KEY_LEFT) {
            jumpRelative(-1);
        } else if (key == KEY_RIGHT) {
            jumpRelative(1);
        } else if (key >= KEY_F1 and key <= KEY_F12) {
            // slideshow.play_radio_station(key - KEY_F1);
        } else {
            std::cout << "Got unknown key " << key << '\n'; // TODO remove
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

void Slideshow::toggleDebug() {
    mDebug = !mDebug;
}

void Slideshow::drawTime(Color color) {
    // All-C API because the C++ stuff is really bad at this.
    std::time_t now = std::time(nullptr);
    std::tm localTime = *std::localtime(&now);

    char buffer[6];  // Enough for "h:mm\0" or "hh:mm\0"
    std::strftime(buffer, sizeof(buffer), "%-I:%M", &localTime);

    mTextWriter.write(buffer, Vector2 { mScreenWidth - DISPLAY_MARGIN, DISPLAY_MARGIN },
            64, color, TextWriter::Alignment::END, TextWriter::Alignment::START);
}

void Slideshow::drawDebug() {
    constexpr float FONT_SIZE = 30;

    auto cs = getCurrentSlides();

    Vector2 pos { DISPLAY_MARGIN, DISPLAY_MARGIN };

    // Write FPS.
    mTextWriter.write(TextFormat("Frames per second: %i", GetFPS()), pos, FONT_SIZE, WHITE,
            TextWriter::Alignment::START, TextWriter::Alignment::START);
    pos.y += FONT_SIZE;

    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << "Number of photos: " << mDbPhotos.size();
    mTextWriter.write(ss.str().c_str(), pos, FONT_SIZE, WHITE,
            TextWriter::Alignment::START, TextWriter::Alignment::START);
    pos.y += FONT_SIZE;

    mTextWriter.write(TextFormat("Time: %.1fs", mTime), pos, FONT_SIZE, WHITE,
            TextWriter::Alignment::START, TextWriter::Alignment::START);
    pos.y += FONT_SIZE;

    pos.y += FONT_SIZE;

    // Write slide info.
    for (int photoIndex = cs.index - 5; photoIndex <= cs.index + 5; photoIndex++) {
        auto photo = photoByIndex(photoIndex);
        auto slide = mSlideCache.get(photo, false);
        Color color;
        std::stringstream ss;
        ss << photoIndex << ": " << photo;
        if (slide) {
            ss << ", " << *slide;
            color = WHITE;
        } else {
            color = GRAY;
        }
        if (photoIndex == cs.index) {
            color = YELLOW;
        }
        mTextWriter.write(ss.str(), pos, FONT_SIZE, color, TextWriter::Alignment::START, TextWriter::Alignment::START);
        pos.y += FONT_SIZE;
    }
}

void Slideshow::rotatePhoto(int degrees) {
    auto cs = getCurrentSlides();

    if (cs.currentSlide && !cs.currentSlide->isBroken()) {
        auto slide = cs.currentSlide;

        slide->photo().rotation += degrees;
        slide->persistState(mDatabase);
        slide->computeIdealSize(mScreenWidth, mScreenHeight);
        jumpRelative(0);
    }
}

void Slideshow::ratePhoto(int rating) {
    auto cs = getCurrentSlides();

    if (cs.currentSlide && !cs.currentSlide->isBroken()) {
        auto slide = cs.currentSlide;

        slide->photo().rating = rating;
        slide->persistState(mDatabase);
    }
}

std::shared_ptr<Image> Slideshow::makeBrokenImage(TextWriter &textWriter) {
    Image image = GenImageGradientRadial(1024, 1024, 0.0f,
            ColorFromHSV(20.0f, 0.2f, 0.2f),
            ColorFromHSV(20.0f, 0.2f, 0.1f));
    std::shared_ptr<Image> textImage = textWriter.makeImage("broken image", 128, WHITE);
    float width = textImage->width;
    float height = textImage->height;
    Rectangle srcRec = { 0.0f, 0.0f, width, height };
    Rectangle dstRec = { (image.width - width)/2, (image.height - height)/2, width, height };
    ImageDraw(&image, *textImage, srcRec, dstRec, WHITE);
    return makeImageSharedPtr(image);
}

