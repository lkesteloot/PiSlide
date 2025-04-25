
#pragma once

#include <algorithm>

#include "raylib.h"

#include "database.h"
#include "util.h"
#include "constants.h"

/**
 * A displayed slide. TODO move much of this to a .cpp file.
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
    void computeIdealSize(int screenWidth, int screenHeight) {
        int textureWidth = mTexture.width;
        int textureHeight = mTexture.height;

        int angle = modulo(mPhoto.rotation, 360);
        bool rotated = angle == 90 || angle == 270;

        if (rotated) {
            // Swap, it's rotated on its side.
            std::swap(textureWidth, textureHeight);
        }

        int width, height;
        if (screenWidth*textureHeight > textureWidth*screenHeight) {
            // Screen is wider than photo.
            height = screenHeight;
            width = textureWidth*height/textureHeight;
        } else {
            // Photo is wider than screen.
            width = screenWidth;
            height = textureHeight*width/textureWidth;
        }

        if (rotated) {
            // Swap back.
            std::swap(width, height);
        }

        mIdealWidth = width;
        mIdealHeight = height;
    }

    /**
     * Move toward our ideal values and update the sprite position. "time" is relative
     * to start of display of slide, where 0 means it's fully visible and negative
     * means it's fading in.
     */
    void move(bool paused, bool promptingEmail, double time) {
        // How much to move toward target.
        float step;
        if (mConfigured) {
            step = 0.3;
        } else {
            // Full jump if not configured.
            step = 1.0;
            mConfigured = true;
        }

        mActualRotate = interpolate(mActualRotate, mPhoto.rotation, step);
        mActualWidth = interpolate(mActualWidth, mIdealWidth, step);
        mActualHeight = interpolate(mActualHeight, mIdealHeight, step);

        // Figure out our targets.
        float idealZoom, idealAlpha;
        if (paused) {
            idealZoom = 1.0;
            idealAlpha = time >= 0 ? (promptingEmail ? 0.2 : 1.0) : 0.0;
            // self.show_labels = time >= 0;
        } else {
            float t = time/SLIDE_DISPLAY_S;
            idealZoom = interpolate(mStartZoom, mEndZoom, mSwapZoom.value_or(false) ? 1 - t : t);

            if (time < 0) {
                idealAlpha = std::clamp((time + SLIDE_TRANSITION_S)/SLIDE_TRANSITION_S, 0.0, 1.0);
                // self.show_labels = false;
            } else if (time < SLIDE_DISPLAY_S - SLIDE_TRANSITION_S) {
                idealAlpha = 1.0;
                // self.show_labels = true;
            } else {
                idealAlpha = std::clamp((SLIDE_DISPLAY_S - time)/SLIDE_TRANSITION_S, 0.0, 1.0);
                // self.show_labels = false
            }
        }

        mActualZoom = interpolate(mActualZoom, idealZoom, step);

        // Alpha is always slow.
        mActualAlpha = interpolate(mActualAlpha, idealAlpha, 0.3);

        touch();
    }

    /**
     * Draw the slide, its labels, and the star rating.
     */
    void draw(int screenWidth, int screenHeight) {
        // self.scale(mActualWidth*mActualZoom, mActualHeight*mActualZoom, 1.0)
        float scale = mActualZoom*mActualWidth/mTexture.width;
        float x = (screenWidth - mTexture.width*scale)/2;
        float y = (screenHeight - mTexture.height*scale)/2;
        DrawTextureEx(mTexture, Vector2 { x, y }, 0, scale, Fade(WHITE, mActualAlpha));
        /*
        if self.show_labels {
            self.name_label.draw()
            self.date_label.draw()

            if self.photo.rating != 3 {
                for i in range(self.photo.rating) {
                    self.star_sprite.scale(STAR_SCALE, STAR_SCALE, 1)
                    self.star_sprite.position((-(self.photo.rating - 1)/2.0 + i)*STAR_SCALE*1.2 + STAR_OFFSET, -470, 0.05)
                    self.star_sprite.draw()
                    */
        touch();
    }

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
    void touch() {
        mLastUsed = now();
    }
};
