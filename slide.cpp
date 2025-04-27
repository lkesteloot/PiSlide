
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
        mShowLabels = time >= 0;
    } else {
        float t = time/SLIDE_DISPLAY_S;
        idealZoom = interpolate(mStartZoom, mEndZoom, mSwapZoom.value_or(false) ? 1 - t : t);

        if (time < 0) {
            idealAlpha = std::clamp((time + SLIDE_TRANSITION_S)/SLIDE_TRANSITION_S, 0.0, 1.0);
            mShowLabels = false;
        } else if (time < SLIDE_DISPLAY_S - SLIDE_TRANSITION_S) {
            idealAlpha = 1.0;
            mShowLabels = true;
        } else {
            idealAlpha = std::clamp((SLIDE_DISPLAY_S - time)/SLIDE_TRANSITION_S, 0.0, 1.0);
            mShowLabels = false;
        }
    }

    mActualZoom = interpolate(mActualZoom, idealZoom, step);

    // Alpha is always slow.
    mActualAlpha = interpolate(mActualAlpha, idealAlpha, 0.3);

    touch();
}

void Slide::draw(TextWriter &textWriter, int screenWidth, int screenHeight) {
    // Draw the photo.
    float scale = mActualZoom*mActualWidth/mTexture.width;
    float x = (screenWidth - mTexture.width*scale)/2;
    float y = (screenHeight - mTexture.height*scale)/2;
    DrawTextureEx(mTexture, Vector2 { x, y }, 0, scale, Fade(WHITE, mActualAlpha));

    // Square the alpha to bias towards transparent, because overlapping text
    // looks bad and we want more transparency during the cross-fade.
    Color color = Fade(WHITE, mActualAlpha*mActualAlpha);
    textWriter.write(mPhoto.label, Vector2 { screenWidth/2.0f, screenHeight - 320.0f },
            48, color, TextWriter::Alignment::CENTER, TextWriter::Alignment::START);
    textWriter.write(mPhoto.displayDate, Vector2 { screenWidth/2.0f, screenHeight - 265.0f },
            32, color, TextWriter::Alignment::CENTER, TextWriter::Alignment::START);

    /*
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
