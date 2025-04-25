
#pragma once

#include <vector>

#include "database.h"

class Slideshow {
    std::vector<Photo> const &mDbPhotos;
    int mScreenWidth;
    int mScreenHeight;
    Database const &mDatabase;

public:
    Slideshow(std::vector<Photo> const &dbPhotos,
            int screenWidth,
            int screenHeight,
            Database const &database)
        : mDbPhotos(dbPhotos),
        mScreenWidth(screenWidth),
        mScreenHeight(screenHeight),
        mDatabase(database) {}
        // self.slide_loader = SlideLoader(shader, star_sprite, self.screen_width, self.screen_height)
        // self.slide_loader.start()

        // self.slide_cache = SlideCache(self.slide_loader)
    virtual ~Slideshow() {}

    // Can't copy.
    Slideshow(const Slideshow &) = delete;
    Slideshow &operator=(const Slideshow &) = delete;

    bool loopRunning() const;
    void move();
    void draw();
};

