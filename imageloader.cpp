
#include <functional>

#include "imageloader.h"

// Unload and delete the image.
void deleteImage(Image *image) {
    UnloadImage(*image);
    delete image;
}

ImageLoader::ImageLoader()
    : mExecutor(1, std::bind(&ImageLoader::loadPhotoInThread, this, std::placeholders::_1)) {}

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
    }

    return images;
}

ImageLoader::Response ImageLoader::loadPhotoInThread(ImageLoader::Request const &request) {
    Image image = LoadImage(request.photo.absolutePathname.c_str());

    // Move the Image object to the heap.
    Image *imagePtr = new Image;
    *imagePtr = image;

    return Response {
        .photo = request.photo,
        .image = std::shared_ptr<Image>(imagePtr, deleteImage),
    };
}
