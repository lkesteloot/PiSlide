
#include <functional>

#include "imageloader.h"
#include "constants.h"

namespace {
    /**
     * Replace the "size" pixels at the border of the image
     * with transparent pixels.
     */
    void drawTransparentBorder(Image *image, int size) {
        int width = image->width;
        int height = image->height;

        // Top.
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < width; x++) {
                ImageDrawPixel(image, x, y, BLANK);
            }
        }

        // Bottom.
        for (int y = height - size; y < height; y++) {
            for (int x = 0; x < width; x++) {
                ImageDrawPixel(image, x, y, BLANK);
            }
        }

        for (int y = 0; y < height; y++) {
            // Left.
            for (int x = 0; x < size; x++) {
                ImageDrawPixel(image, x, y, BLANK);
            }

            // Right.
            for (int x = width - size; x < width; x++) {
                ImageDrawPixel(image, x, y, BLANK);
            }
        }
    }
}

ImageLoader::ImageLoader()
    : mExecutor(1, []<typename T0>(T0 && PH1) { return loadPhotoInThread(std::forward<T0>(PH1)); }) {}

void ImageLoader::requestImage(Photo const &photo) {
    if (!mAlreadyRequestedIds.contains(photo.id)) {
        mAlreadyRequestedIds.emplace(photo.id);
        mExecutor.ask(Request { photo });
    }
}

std::vector<LoadedImage> ImageLoader::getLoadedImages() {
    std::vector<LoadedImage> loadedImages;

    while (true) {
        std::optional<Response> response = mExecutor.get();
        if (!response.has_value()) {
            break;
        }

        loadedImages.emplace_back(LoadedImage {
            .photo = response->photo,
            .image = response->image,
            .loadTime = response->loadTime,
        });
        mAlreadyRequestedIds.erase(response->photo.id);
    }

    return loadedImages;
}

ImageLoader::Response ImageLoader::loadPhotoInThread(Request const &request) {
    auto beginTime = std::chrono::high_resolution_clock::now();
    Image image = LoadImage(request.photo.absolutePathname.c_str());
    auto endTime = std::chrono::high_resolution_clock::now();

    std::shared_ptr<Image> imagePtr;
    if (IsImageValid(image)) {
        // We don't get good anti-aliasing at the edge of the image, so make
        // the border transparent, which anti-aliases much better.
        drawTransparentBorder(&image, TRANSPARENT_BORDER);

        // TODO see if we can compute the mipmaps here. Compare with doing it
        // in the GPU later on the Texture object.

        imagePtr = makeImageSharedPtr(image);
    } else {
        // Image failed to load.
        std::cerr << "Failed to load " << request.photo.absolutePathname << '\n';
    }

    return Response {
        .photo = request.photo,
        .image = imagePtr,
        .loadTime = endTime - beginTime,
    };
}
