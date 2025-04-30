
#pragma once

// For debugging, should normally be False:
constexpr bool QUICK_SPEED = false;

// The display time includes only one transition (the end one):
constexpr float SLIDE_DISPLAY_S = QUICK_SPEED ? 5 : 14;
constexpr float SLIDE_TRANSITION_S = 2;

constexpr double MAX_PAUSE_SECONDS = 60*60;

constexpr float DISPLAY_MARGIN = 50;

constexpr float STAR_SIZE = 30;
