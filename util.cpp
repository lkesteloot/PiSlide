
#include <chrono>

#include "util.h"

int modulo(int a, int b) {
    int result = a % b;
    return result < 0 ? result + b : result;
}

float interpolate(float a, float b, float t) {
    return a + (b - a)*t;
}

double now() {
    std::chrono::duration<double> now = std::chrono::steady_clock::now().time_since_epoch();
    return now.count();
}
