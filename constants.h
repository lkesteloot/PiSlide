
#pragma once

// For debugging, should normally be False:
constexpr bool QUICK_SPEED = false;

// The display time includes only one transition (the end one):
constexpr float SLIDE_DISPLAY_S = QUICK_SPEED ? 5 : 14;
constexpr float SLIDE_TRANSITION_S = 2;

constexpr double MAX_PAUSE_SECONDS = 60*60;

constexpr float DISPLAY_MARGIN = 50;

constexpr float STAR_SIZE = 30;

// Number of pixels to make transparent at the border of the image to
// avoid visual artifacts during animation.
constexpr int TRANSPARENT_BORDER = 4;

// Number of slides in the cache.
constexpr int SLIDE_CACHE_SIZE = 4;
