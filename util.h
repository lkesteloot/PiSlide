
#pragma once

#include <ctime>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <cstddef>   // for std::byte

#include "raylib.h"

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
double nowArbitrary();

/**
 * Time in seconds since the epoch.
 */
time_t nowEpoch();

/**
 * Make shared_ptr versions of raylib structures. When the last reference
 * is lost, the structures will be unloaded.
 */
std::shared_ptr<Image> makeImageSharedPtr(Image image);
std::shared_ptr<Font> makeFontSharedPtr(Font font);

/**
 * Split the input at the delimiter, keeping empty parts. Will always
 * contain at least one part.
 */
std::vector<std::string> split(std::string const &input, char delimiter);

/**
 * Return the pathname without its extension. Removes the dot as well. Does not
 * count dots at the beginning of the filename as starting an extension.
 *
 * Input: a/b/c.d
 * Output: a/b/c
 */
std::string stripExtension(std::string const &pathname);

/**
 * Read all the bytes of a file. Throws runtime_error() if it can't
 * open or read the file.
 */
std::vector<std::byte> readFileBytes(std::filesystem::path const &path);

/**
 * Compute the hex of the SHA-1 of the bytes.
 */
std::string sha1Hex(void const *data, size_t byteCount);

