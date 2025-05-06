
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

double now() {
    std::chrono::duration<double> now = std::chrono::steady_clock::now().time_since_epoch();
    return now.count();
}


std::shared_ptr<Image> makeImageSharedPtr(Image image) {
    return std::shared_ptr<Image>(new Image(image), deleteImage);
}

std::shared_ptr<Font> makeFontSharedPtr(Font font) {
    return std::shared_ptr<Font>(new Font(font), deleteFont);
}
