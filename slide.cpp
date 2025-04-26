
#include "slide.h"
#include "util.h"
#include "constants.h"

void Slide::computeIdealSize(int screenWidth, int screenHeight) {
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

void Slide::move(bool paused, bool promptingEmail, double time) {
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

void Slide::draw(int screenWidth, int screenHeight) {
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

void Slide::touch() {
    mLastUsed = now();
}
