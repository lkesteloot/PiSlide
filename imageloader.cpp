
#include <functional>

#include "imageloader.h"

namespace {
    // Unload and delete the image.
    void deleteImage(const Image *image) {
        UnloadImage(*image);
        delete image;
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

    // Move the Image object to the heap. (Lint says the memory is leaked,
    // but I think this is a false positive.)
    auto *heapImage = new Image(image);

    return Response {
        .photo = request.photo,
        .image = std::shared_ptr<Image>(heapImage, deleteImage),
        .loadTime = endTime - beginTime,
    };
}
