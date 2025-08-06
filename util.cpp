
#include <string_view>
#include <fstream>
#include <stdexcept>

#include "TinySHA1.hpp"

#include "util.h"

namespace {
    // Unload and delete the image.
    void deleteImage(const Image *image) {
        UnloadImage(*image);
        delete image;
    }

    // Unload and delete the font object.
    void deleteFont(const Font *font) {
        UnloadFont(*font);
        delete font;
    }
}

int modulo(int a, int b) {
    int result = a % b;
    return result < 0 ? result + b : result;
}

float interpolate(float a, float b, float t) {
    return a + (b - a)*t;
}

double nowArbitrary() {
    std::chrono::duration<double> now = std::chrono::steady_clock::now().time_since_epoch();
    return now.count();
}

time_t nowEpoch() {
    return std::time(0);
}

std::shared_ptr<Image> makeImageSharedPtr(Image image) {
    return std::shared_ptr<Image>(new Image(image), deleteImage);
}

std::shared_ptr<Font> makeFontSharedPtr(Font font) {
    return std::shared_ptr<Font>(new Font(font), deleteFont);
}

std::vector<std::string> split(std::string const &input, char delimiter) {
    std::vector<std::string> result;
    std::string_view view{input};

    while (!view.empty()) {
        auto pos = view.find(delimiter);
        if (pos == std::string_view::npos) {
            result.emplace_back(view);
            break;
        }
        result.emplace_back(view.substr(0, pos));
        view.remove_prefix(pos + 1);
    }

    return result;
}

std::string stripExtension(std::string const &pathname) {
    // Find the filename, so we don't find dots in directory components.
    auto filename = pathname.rfind('/');
    if (filename == std::string::npos) {
        filename = 0;
    } else {
        filename += 1;
    }

    // Find the last dot of the filename.
    auto dot = pathname.rfind('.');
    if (dot == std::string::npos || dot <= filename + 1) {
        return pathname;
    } else {
        return pathname.substr(0, dot);
    }
}

std::vector<std::byte> readFileBytes(std::filesystem::path const &path) {
    // "ate" positions the pointer at the end of the file.
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Couldn't open file");
    }

    std::streamsize size = file.tellg();
    file.seekg(0);

    std::vector<std::byte> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Couldn't read file");
    }

    return buffer;
}

std::string sha1Hex(void const *data, size_t byteCount) {
    sha1::SHA1 s;
    s.processBytes(data, byteCount);

    uint32_t digest[5];
    s.getDigest(digest);

    char hex[41];
    snprintf(hex, sizeof(hex), "%08x%08x%08x%08x%08x",
            digest[0], digest[1], digest[2], digest[3], digest[4]);

    return hex;
}

