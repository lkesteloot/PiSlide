
#include <cmath>

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

void Slide::move(Config const &config, bool paused, bool promptingEmail, double time) {
    // How much to move toward target.
    float step;
    if (mConfigured) {
        step = 0.3;
    } else {
        // Full jump if not configured.
        step = 1.0;
        mConfigured = true;
    }

    mActualRotate = interpolate(mActualRotate, isBroken() ? 0 : mPhoto.rotation, step);
    mActualWidth = interpolate(mActualWidth, mIdealWidth, step);
    mActualHeight = interpolate(mActualHeight, mIdealHeight, step);

    // Figure out our targets.
    float idealZoom, idealAlpha;
    if (paused) {
        idealZoom = 1.0;
        idealAlpha = time >= 0 ? (promptingEmail ? 0.2f : 1.0f) : 0.0f;
        mShowLabels = time >= 0;
    } else {
        float slideTotalTime = config.slideTotalTime();
        float slideTransitionTime = config.slideTransitionTime;
        float t = time/slideTotalTime;
        idealZoom = interpolate(mStartZoom, mEndZoom, mSwapZoom.value_or(false) ? 1 - t : t);

        if (time < 0) {
            idealAlpha = std::clamp((time + slideTransitionTime)/slideTransitionTime, 0.0, 1.0);
            mShowLabels = false;
        } else if (time < slideTotalTime - slideTransitionTime) {
            idealAlpha = 1.0;
            mShowLabels = true;
        } else {
            idealAlpha = std::clamp((slideTotalTime - time)/slideTransitionTime, 0.0, 1.0);
            mShowLabels = false;
        }
    }

    mActualZoom = interpolate(mActualZoom, idealZoom, step);

    // Alpha is always slow.
    mActualAlpha = interpolate(mActualAlpha, idealAlpha, 0.3);

    touch();
}

void Slide::draw(Config const &config, TextWriter &textWriter, Texture const &starTexture,
        int screenWidth, int screenHeight, float opacity, bool drawText) {

    // Draw the photo.
    float scale = mActualZoom*mActualWidth/mTexture.width;
    // Negate angle to be compatible with Python version and data in database:
    float angle = -mActualRotate;
    float s = sin(angle*DEG2RAD);
    float c = cos(angle*DEG2RAD);
    float screenCenterX = screenWidth/2;
    float screenCenterY = screenHeight/2;
    float photoCenterX = mTexture.width*scale/2;
    float photoCenterY = mTexture.height*scale/2;
    float x = screenCenterX - (photoCenterX*c - photoCenterY*s);
    float y = screenCenterY - (photoCenterX*s + photoCenterY*c);
    DrawTextureEx(mTexture, Vector2 { x, y }, angle, scale, Fade(WHITE, mActualAlpha*opacity));

    // We're drawing bottom to top.
    y = screenHeight - DISPLAY_MARGIN;

    // Draw the rating stars.
    y -= STAR_SIZE;
    int rating = mPhoto.rating;
    if (rating != 3) {
        float starScale = STAR_SIZE / starTexture.width;
        Color starColor = Fade(WHITE, 0.5*mActualAlpha*mActualAlpha*opacity);

        for (int i = 0; i < rating; i++) {
            Vector2 position {
                .x = screenWidth/2 + (-(rating - 1)/2.0f + i)*STAR_SIZE*1.2f - STAR_SIZE/2.0f,
                .y = y,
            };
            DrawTextureEx(starTexture, position, 0, starScale, starColor);
        }
    }
    y -= 20;

    // Draw the text.
    if (drawText) {
        // Square the alpha to bias towards transparent, because overlapping text
        // looks bad and we want more transparency during the cross-fade.
        Color color = Fade(WHITE, mActualAlpha*mActualAlpha*opacity);
        Rectangle pos = textWriter.write(mPhoto.displayDate, Vector2 { screenWidth/2.0f, y },
                32, color, TextWriter::Alignment::CENTER, TextWriter::Alignment::END);
        y -= 4 + pos.height;
        textWriter.write(mPhoto.label, Vector2 { screenWidth/2.0f, y },
                48, color, TextWriter::Alignment::CENTER, TextWriter::Alignment::END);
    }

    touch();
}

void Slide::touch() {
    mLastUsed = nowArbitrary();
}

std::ostream &operator<<(std::ostream &os, Slide const &slide) {
    os << "load " << std::chrono::duration_cast<std::chrono::milliseconds>(slide.mLoadTime)
        << ", prep " << std::chrono::duration_cast<std::chrono::milliseconds>(slide.mPrepTime);
    if (slide.isBroken()) {
        os << ", broken";
    }
    return os;
}
