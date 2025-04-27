
#pragma once

// For debugging, should normally be False:
constexpr bool QUICK_SPEED = true;

// The display time includes only one transition (the end one):
constexpr float SLIDE_DISPLAY_S = QUICK_SPEED ? 5 : 14;
constexpr float SLIDE_TRANSITION_S = 2;

