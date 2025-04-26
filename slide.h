
#pragma once

#include <algorithm>
#include <optional>

#include "raylib.h"

#include "database.h"

/**
 * A displayed slide.
 */
class Slide {
    Photo mPhoto;
    Texture mTexture; // We own this.
    bool mIsBroken = false;

    // What we're drawing.
    float mActualRotate = 0;
    float mActualWidth = 0;
    float mActualHeight = 0;
    float mActualZoom = 0;
    float mActualAlpha = 0;

    // Filled in later in computeIdealSize().
    int mIdealWidth = 0;
    int mIdealHeight = 0;

    // For the zoom.
    float mStartZoom = 0.9;
    float mEndZoom = 1.3; // Clip off a bit.
    std::optional<bool> mSwapZoom;

    // We've not moved yet, so our actuals are bogus.
    bool mConfigured = false;

    // When we were last used (drawn, moved, requested).
    double mLastUsed = 0;

public:
    Slide(Photo const &photo, Texture texture) : mPhoto(photo), mTexture(texture) {}
    virtual ~Slide() {
        UnloadTexture(mTexture);
    }

    // Can't copy, would unload the texture multiple times.
    Slide(const Slide &) = delete;
    Slide &operator=(const Slide &) = delete;

    /*
        show_twilio_instructions = config.PARTY_MODE and config.TWILIO_SID
        label = config.TWILIO_MESSAGE if show_twilio_instructions else self.photo.label
        self.name_label = pi3d.String(font=TEXT_FONT, string=label,
                is_3d=false, x=0, y=-360, z=0.05)
        self.name_label.set_shader(shader)

        display_date = "" if show_twilio_instructions else self.photo.display_date
        self.date_label = pi3d.String(font=DATE_FONT, string=display_date,
                is_3d=false, x=0, y=-420, z=0.05)
        self.date_label.set_shader(shader)
        */

        // self.show_labels = false

    /**
     * Save our own state to the database.
     */
    void persistState(Database const &database) {
        // database.savePhoto(mPhoto);
    }

    /**
     * Compute the ideal width and height given the screen size and the rotation
     * to fit the photo as large as possible.
     */
    void computeIdealSize(int screenWidth, int screenHeight);

    /**
     * Move toward our ideal values and update the sprite position. "time" is relative
     * to start of display of slide, where 0 means it's fully visible and negative
     * means it's fading in.
     */
    void move(bool paused, bool promptingEmail, double time);

    /**
     * Draw the slide, its labels, and the star rating.
     */
    void draw(int screenWidth, int screenHeight);

    /**
     * Pretend we've not been drawn so that the next draw will start from scratch.
     */
    void turnOff() {
        mConfigured = false;
        mActualAlpha = 0.0;
    }

    /**
     * Record that we've been used (moved, drawn).
     */
    void touch();

    /**
     * When this slide was last touched.
     */
    double lastUsed() const {
        return mLastUsed;
    }

    /**
     * Whether we're zooming in or out, or unset.
     */
    std::optional<bool> swapZoom() const {
        return mSwapZoom;
    }

    /**
     * Set whether we're going in or out.
     */
    void setSwapZoom(bool swapZoom) {
        mSwapZoom = swapZoom;
    }

    bool isBroken() const {
        return mIsBroken;
    }

    bool configured() const {
        return mConfigured;
    }
};
