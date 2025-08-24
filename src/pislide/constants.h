
#pragma once

constexpr float DISPLAY_MARGIN = 50;

constexpr float STAR_SIZE = 30;

// Number of pixels to make transparent at the border of the image to
// avoid visual artifacts during animation.
constexpr int TRANSPARENT_BORDER = 4;

// Number of slides in the cache.
constexpr int SLIDE_CACHE_SIZE = 4;

// Seconds between fetches of bus info.
constexpr int BUS_INFO_FETCH_S = 60;

// Maximum width or height of an image. Those bigger than this get downsized.
// On the Raspberry Pi 5 the max texture size is 4096 according to
// `glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);`, and in any case we don't
// need anything bigger than 2048 for our displays. It might be nice eventually
// to auto-detect the GL max size, but raylib doesn't provide that.
constexpr int MAX_TEXTURE_SIZE = 2048;

// Minimum number of seconds between Twilio fetches during party mode.
constexpr double MIN_TWILIO_FETCH_INTERVAL_S = 1;

