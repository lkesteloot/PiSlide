
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

std::vector<std::pair<Photo,std::shared_ptr<Image>>> ImageLoader::getImages() {
    std::vector<std::pair<Photo,std::shared_ptr<Image>>> images;

    while (true) {
        std::optional<Response> response = mExecutor.get();
        if (!response.has_value()) {
            break;
        }

        images.emplace_back(response->photo, response->image);
        mAlreadyRequestedIds.erase(response->photo.id);
    }

    return images;
}

ImageLoader::Response ImageLoader::loadPhotoInThread(Request const &request) {
    Image image = LoadImage(request.photo.absolutePathname.c_str());

    // Move the Image object to the heap. (Lint says the memory is leaked,
    // but I think this is a false positive.)
    auto *heapImage = new Image(image);

    return Response {
        .photo = request.photo,
        .image = std::shared_ptr<Image>(heapImage, deleteImage),
    };
}
