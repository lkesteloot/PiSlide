
#pragma once

#include "raylib.h"

#include <chrono>
#include <memory>

/**
 * For timing stuff we do, like loading an image.
 */
typedef std::chrono::high_resolution_clock::duration Timing;

/**
 * Computes a modulo b. Assumes b is positive. The
 * result is always between 0 (inclusive) and b (exclusive).
 */
int modulo(int a, int b);

/**
 * Return a number that is t (which is 0 to 1) between a and b.
 */
float interpolate(float a, float b, float t);

/**
 * Time in seconds since some arbitrary time.
 */
double now();

/**
 * Make shared_ptr versions of raylib structures. When the last reference
 * is lost, the structures will be unloaded.
 */
std::shared_ptr<Image> makeImageSharedPtr(Image image);
std::shared_ptr<Font> makeFontSharedPtr(Font font);
