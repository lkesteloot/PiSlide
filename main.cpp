
#include <stdio.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <string>
#include <vector>
#include <fts.h>
#include <sys/stat.h>
#include <cstring>
#include <set>
#include <map>

#include "raylib.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raymath.h"
#pragma GCC diagnostic pop

#include "database.h"
#include "slideshow.h"
#include "executor.h"

namespace {
    constexpr int MAX_NO_FILE_WARNINGS = 10;

    /**
     * Directories to skip altogether.
     */
    std::set<std::string> UNWANTED_DIRS = {
        ".thumbnails",
        ".small",
        "Broken",
        "Private",
    };
}

/**
 * Whether the pathname refers to an image that we want to show.
 */
bool isImagePathname(std::string const &pathname) {
    const char *ext = GetFileExtension(pathname.c_str());

    // It's surprisingly hard to do this in C++.
    return ext != nullptr && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0);
}

/*
 * Gets a set of all pathnames in image directory. These are relative to the
 * passed-in root dir.
 */
std::set<std::string> traverseDirectoryTree(std::string const &rootDir) {
    std::set<std::string> pathnames;

    if (rootDir.ends_with("/")) {
        throw std::invalid_argument("rootDir must not end with a slash");
    }

    char *paths[] = { const_cast<char*>(rootDir.c_str()), nullptr };

    FTS *ftsp = fts_open(paths, FTS_LOGICAL | FTS_NOSTAT, nullptr);
    if (ftsp == nullptr) {
        perror("fts_open");
        return pathnames;
    }

    FTSENT *ent;
    while ((ent = fts_read(ftsp)) != nullptr) {
        switch (ent->fts_info) {
            case FTS_D: {
                // std::cout << "Entering directory: " << ent->fts_path << std::endl;
                std::string dirname = GetFileName(ent->fts_path);
                if (UNWANTED_DIRS.contains(dirname)) {
                    fts_set(ftsp, ent, FTS_SKIP);
                }
                break;
            }

            case FTS_F:
            case FTS_NSOK: {
                // std::cout << "File: " << ent->fts_path << std::endl;
                std::string path = ent->fts_path;
                if (isImagePathname(path)) {
                    // Strip rootDir.
                    if (path.starts_with(rootDir)) {
                        std::string subpath = path.substr(rootDir.size());
                        while (subpath.starts_with("/")) {
                            subpath = subpath.substr(1);
                        }
                        pathnames.insert(subpath);
                    } else {
                        throw std::invalid_argument(std::string("path does not start with rootDir: " + path));
                    }
                }
                break;
            }

            case FTS_DP:
                // std::cout << "Leaving directory: " << ent->fts_path << std::endl;
                break;

            case FTS_SL:
                // TODO
                std::cout << "Symbolic link: " << ent->fts_path << std::endl;
                break;

            case FTS_ERR:
                // TODO
                std::cerr << "Error: " << ent->fts_path << ": " <<
                    strerror(ent->fts_errno) << std::endl;
                break;

            default:
                break;
        }
    }

    if (errno != 0) {
        perror("fts_read");
    }

    if (fts_close(ftsp) != 0) {
        perror("fts_close");
    }

    return pathnames;
}

/**
 * Assign a pathname to each photo, returning a new copy of dbPhotos with
 * invalid photos (those with no disk files) removed.
 */
std::vector<Photo> assignPhotoPathnames(Database const &database,
        std::vector<Photo> const &dbPhotos,
        std::set<std::string> const &diskPathnames) {

    // Get all photo files. We did this before but have since modified the database.
    std::vector<PhotoFile> dbPhotoFiles = database.getAllPhotoFiles();

    // Map from hash to list of photo files.
    std::multimap<std::string, PhotoFile *> photoFileMap;
    for (auto &photoFile : dbPhotoFiles) {
        photoFileMap.insert({photoFile.hashBack, &photoFile});
    }

    int warningCount = 0;

    // Handle each photo, making new array of photos.
    std::vector<Photo> goodPhotos;
    for (auto &photo : dbPhotos) {
        // Try every photo file.
        bool found = false;
        auto [begin, end] = photoFileMap.equal_range(photo.hashBack);
        for (auto photoFile = begin; photoFile != end; photoFile++) {
            // See if it's on disk.
            if (diskPathnames.contains(photoFile->second->pathname)) {
                Photo newPhoto = photo;
                newPhoto.pathname = photoFile->second->pathname;
                goodPhotos.push_back(newPhoto);
                found = true;
                break;
            }
        }
        if (!found) {
            // Can't find any file on disk for this photo.
            warningCount += 1;
            if (warningCount <= MAX_NO_FILE_WARNINGS) {
                std::cout << "No file on disk for " << photo.hashBack << " (" << photo.label << ")" << std::endl;
            }
        }
    }

    if (warningCount != 0) {
        std::cout << "Files missing on disk: " << warningCount << std::endl;
    }

    return goodPhotos;
}

/**
 * Like main(), but is allowed to throw, and the exception will be displayed
 * nicely to the user.
 */
int main_can_throw() {
    Database database;

    // Write integers with thousands separators.
    std::cout.imbue(std::locale(""));

    // TODO parse command-line arguments.
    // TODO parse config file.
    // std::string rootPath = "/Users/lk/Dropbox/Team Ten Photos/";
    std::string rootPath = "/Users/lk/tmp/pislide-root";
    // TODO upgrade the schema.

    // Recursively read the photo tree from disk.
    // std::string rootPath = "/Users/lk/Dropbox/Team Ten Photos/Summer of Love";
    std::set<std::string> diskPathnames = traverseDirectoryTree(rootPath);
    std::cout << "Photos on disk: " << diskPathnames.size() << std::endl;

    // TODO optionally resize images.

    // Get all photo files from the database.
    std::vector<PhotoFile> dbPhotoFiles = database.getAllPhotoFiles();
    std::cout << "Photo files in database: " << dbPhotoFiles.size() << std::endl;

    // TODO Compute missing hashes of photos, add them to database.
    // handle_new_and_renamed_files(con, diskPathnames, db_photo_files)

    // Keep only photos of the right rating and date range.
    std::vector<Photo> dbPhotos = database.getAllPhotos();
    std::cout << "Photos in database: " << dbPhotos.size() << std::endl;
    // dbPhotos = filter_photos_by_rating(dbPhotos, args.min_rating)
    // print("Photos after rating filter: %d" % (len(dbPhotos),))
    // dbPhotos = filter_photos_by_date(dbPhotos, args.min_days, args.max_days)
    // print("Photos after date filter: %d" % (len(dbPhotos),))

    // Find a pathname for each photo.
    dbPhotos = assignPhotoPathnames(database, dbPhotos, diskPathnames);

    // print("Photos after disk filter: %d" % (len(dbPhotos),))
    // dbPhotos = filter_photos_by_substring(dbPhotos, args.includes)
    // print("Photos after dir filter: %d" % (len(dbPhotos),))

    std::cout << "Final photos to be shown: " << dbPhotos.size() << std::endl;

    if (dbPhotos.empty()) {
        std::cerr << "Error: No photos found." << std::endl;
        return -1;
    }

    // Shuffle the photos.
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(dbPhotos.begin(), dbPhotos.end(), gen);

    Executor<int,int> executor(4, [](int x) { return x * 2; });
    executor.ask(7);
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    std::optional<int> qq = executor.get();
    if (qq.has_value()) {
        std::cout << "Value: " << *qq << std::endl;
    } else {
        std::cout << "No value" << std::endl;
    }

    return 0;
    // SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_FULLSCREEN_MODE);
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(0, 0, "PiSlide");

    // This is really the window size:
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    std::cout << "Window size: " << screenWidth << "x" << screenHeight << std::endl;

    // Load the star icon.
    /*
    star_texture = pi3d.Texture("outline-star-256.png", blend=True, mipmap=True)
    star_sprite = pi3d.ImageSprite(star_texture, shader, camera=camera, w=1.0, h=1.0, z=0.05)
    star_sprite.set_alpha(0.5)
    LOGGER.info("Star icon: %dx%d" % (star_texture.ix, star_texture.iy))
    */

    // Match Python version.
    SetTargetFPS(40);

    Slideshow slideshow(dbPhotos, screenWidth, screenHeight, database);

    while (slideshow.loopRunning()) {
        // slideshow.prefetch(MAX_CACHE_SIZE/2 + 1);
        slideshow.move();
        // slideshow.fetch_twilio_photos();
        slideshow.draw();

        /*
        key = keyboard.read()
        if key != -1:
            took_key = slideshow.take_key(key)
            if not took_key:
                if key == ord("Q"):
                    slideshow.shutdown()
                    keyboard.close()
                    display.stop()
                elif key == ord("r"):
                    slideshow.rotate_clockwise()
                elif key == ord("l"):
                    slideshow.rotate_counterclockwise()
                elif key == ord(" "):
                    slideshow.toggle_pause()
                elif key == 261: # Right arrow.
                    slideshow.jump_relative(1)
                elif key == 260: # Left arrow.
                    slideshow.jump_relative(-1)
                elif key == ord("D"):
                    slideshow.toggle_debug()
                elif key == ord("m"):
                    slideshow.mute()
                elif key == ord("s"):
                    slideshow.stop_music()
                elif key == ord("e"):
                    slideshow.prompt_email()
                elif key == ord("b"):
                    slideshow.toggle_bus()
                elif key == ord("T"):
                    slideshow.toggle_twilio()
                elif key >= ord("1") and key <= ord("5"):
                    slideshow.rate_photo(key - ord("1") + 1)
                elif key >= FIRST_FUNCTION_KEY and key < FIRST_FUNCTION_KEY + 10:
                    slideshow.play_radio_station(key - FIRST_FUNCTION_KEY)
                else:
                    LOGGER.info("Got unknown key %d" % key)
                    */
    }

    Texture texture1 = LoadTexture("resources/photo1.jpg");
    Texture texture2 = LoadTexture("resources/photo2.jpg");

    float t = 0;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("Hello Raylib", 200, 200, 20, WHITE);
        {
            float scale = static_cast<float>(screenWidth)/texture1.width*Lerp(0.8, 1.2, t);
            float x = (screenWidth - texture1.width*scale)/2;
            float y = (screenHeight - texture1.height*scale)/2;
            DrawTextureEx(texture1, Vector2 { x, y }, 0, scale, Fade(WHITE, t));
        }
        {
            float scale = static_cast<float>(screenWidth)/texture2.width*Lerp(1.2, 0.8, t);
            float x = (screenWidth - texture2.width*scale)/2;
            float y = (screenHeight - texture2.height*scale)/2;
            DrawTextureEx(texture2, Vector2 { x, y }, 0, scale, Fade(WHITE, 1 - t));
        }
        t += 0.003;
        if (t > 1) {
            t = 1;
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    UnloadTexture(texture1);
    UnloadTexture(texture2);

    CloseWindow();

    return 0;
}

int main() {
    try {
        return main_can_throw();
    } catch (const std::exception &e) {
        std::cerr << "Caught exception (" << e.what() << ")" << std::endl;
    }
    return -1;
}
