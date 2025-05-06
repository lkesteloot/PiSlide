
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <string>
#include <vector>
#include <cstring>
#include <set>
#include <map>
#include <filesystem>

#include "raylib.h"

/* TODO delete?
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raymath.h"
#pragma GCC diagnostic pop
*/

#include "database.h"
#include "slideshow.h"
#include "executor.h"
#include "slidecache.h"
#include "config.h"

namespace {
    constexpr int MAX_FILE_WARNING_COUNT = 10;

    /**
     * Directories to skip altogether. TODO move to config.
     */
    std::set<std::filesystem::path> UNWANTED_DIRS = {
        ".thumbnails",
        ".small",
        "Broken",
        "Private",
    };

    /**
     * Valid image file extensions. Must be lower case.
     */
    std::set<std::string> IMAGE_EXTENSIONS = {
        ".jpeg",
        ".jpg",
    };

    /**
     * Insane that this is so complicated in C++.
     */
    std::string toLowerCase(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    /**
     * Whether the pathname refers to an image that we want to show.
     */
    bool isImagePathname(std::filesystem::path const &pathname) {
        return IMAGE_EXTENSIONS.contains(toLowerCase(pathname.extension().string()));
    }

    /*
     * Gets a set of all pathnames in the image directory. These are relative to the
     * passed-in root dir.
     */
    std::set<std::string> traverseDirectoryTree(std::filesystem::path const &rootDir) {
        std::set<std::string> pathnames;

        if (rootDir.string().ends_with("/")) {
            throw std::invalid_argument("rootDir must not end with a slash");
        }

        try {
            for (auto itr = std::filesystem::recursive_directory_iterator(rootDir,
                    std::filesystem::directory_options::follow_directory_symlink);
                    itr != std::filesystem::recursive_directory_iterator(); ++itr) {

                auto const &entry = *itr;
                std::filesystem::path path = entry.path();
                if (entry.is_regular_file()) {
                    if (isImagePathname(path)) {
                        // Strip rootDir.
                        if (path.string().starts_with(rootDir.string())) {
                            // Create a relative path by removing the rootDir prefix
                            pathnames.insert(path.lexically_relative(rootDir));
                        } else {
                            throw std::invalid_argument(std::string("path does not start with rootDir: ") + path.string());
                        }
                    }
                } else if (entry.is_directory()) {
                    if (UNWANTED_DIRS.contains(path.filename())) {
                        // Don't recurse.
                        itr.disable_recursion_pending();
                    }
                } else {
                    std::cerr << "Unknown directory entry type: " << path << '\n';
                    pathnames.clear();
                    return pathnames;
                }
            }
        } catch (std::filesystem::filesystem_error const &e) {
            std::cerr << "Filesystem error: " << e.what() << '\n';
        }

        return pathnames;
    }

    /**
     * Assign a pathname to each photo, returning a new copy of dbPhotos with
     * invalid photos (those with no disk files) removed.
     */
    std::vector<Photo> assignPhotoPathnames(Database const &database,
            Config const &config,
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

        // Handle each photo, making a new array of photos.
        std::vector<Photo> goodPhotos;
        for (auto &photo : dbPhotos) {
            // Try every photo file.
            bool found = false;
            auto [begin, end] = photoFileMap.equal_range(photo.hashBack);
            for (auto photoFile = begin; photoFile != end; ++photoFile) {
                // See if it's on disk.
                if (diskPathnames.contains(photoFile->second->pathname)) {
                    Photo newPhoto = photo;
                    newPhoto.pathname = photoFile->second->pathname;
                    newPhoto.absolutePathname = config.rootDir / newPhoto.pathname;
                    goodPhotos.push_back(newPhoto);
                    found = true;
                    break;
                }
            }
            if (!found) {
                // Can't find any file on disk for this photo.
                warningCount += 1;
                if (warningCount <= MAX_FILE_WARNING_COUNT) {
                    std::cout << "No file on disk for " << photo.hashBack << " (" << photo.label << ")" << '\n';
                }
            }
        }

        if (warningCount != 0) {
            std::cout << "Files missing on disk: " << warningCount << '\n';
        }

        return goodPhotos;
    }

    std::vector<Photo> filterPhotosByPathnameSubstring(std::vector<Photo> const &dbPhotos) {
        std::vector<Photo> goodPhotos;

        for (auto &photo : dbPhotos) {
            if (true || photo.pathname.find("Florence") != std::string::npos) {
                goodPhotos.push_back(photo);
            }
        }

        return goodPhotos;
    }

    /**
     * Like main(), but is allowed to throw, and the exception will be displayed
     * nicely to the user.
     */
    int mainCanThrow(int argc, char *argv[]) {
        Database database;

        // Raylib is very verbose at INFO level, just keep the warnings.
        SetTraceLogLevel(LOG_WARNING);
        // TODO use SetTraceLogCallback() to redirect raylib logs to a log file.

        // Write integers for humans (commas, etc.).
        std::cout.imbue(std::locale(""));

        // Get our configuration.
        Config config;
        bool success = config.readConfigFile("config.toml");
        if (!success) {
            return 1;
        }
        success = config.parseCommandLine(argc, argv);
        if (!success || !config.isValid()) {
            return 1;
        }


        // TODO upgrade the schema.

        // Recursively read the photo tree from the disk.
        std::set<std::string> diskPathnames = traverseDirectoryTree(config.rootDir);
        std::cout << "Photos on disk: " << diskPathnames.size() << '\n';

        // TODO optionally resize images.

        // Get all photo files from the database.
        std::vector<PhotoFile> dbPhotoFiles = database.getAllPhotoFiles();
        std::cout << "Photo files in database: " << dbPhotoFiles.size() << '\n';

        // TODO Compute missing hashes of photos, add them to database.
        // handle_new_and_renamed_files(con, diskPathnames, db_photo_files)

        // Keep only photos of the right rating and date range.
        std::vector<Photo> dbPhotos = database.getAllPhotos();
        std::cout << "Photos in database: " << dbPhotos.size() << '\n';
        // dbPhotos = filter_photos_by_rating(dbPhotos, args.min_rating)
        // print("Photos after rating filter: %d" % (len(dbPhotos),))
        // dbPhotos = filter_photos_by_date(dbPhotos, args.min_days, args.max_days)
        // print("Photos after date filter: %d" % (len(dbPhotos),))

        // Find a pathname for each photo.
        dbPhotos = assignPhotoPathnames(database, config, dbPhotos, diskPathnames);

        // print("Photos after disk filter: %d" % (len(dbPhotos),))
        dbPhotos = filterPhotosByPathnameSubstring(dbPhotos);
        // print("Photos after dir filter: %d" % (len(dbPhotos),))

        std::cout << "Final photos to be shown: " << dbPhotos.size() << '\n';

        if (dbPhotos.empty()) {
            std::cerr << "Error: No photos found." << '\n';
            return -1;
        }

        // Shuffle the photos.
        std::random_device rd;
        std::mt19937 gen(rd());
        std::ranges::shuffle(dbPhotos, gen);

        // SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_FULLSCREEN_MODE);
        SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
        InitWindow(0, 0, "PiSlide");

        // This is really the window size:
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        std::cout << "Window size: " << screenWidth << "x" << screenHeight << '\n';

        // Load the star icon.
        Texture starTexture = LoadTexture("outline-star-256.png");
        GenTextureMipmaps(&starTexture);
        SetTextureFilter(starTexture, TEXTURE_FILTER_TRILINEAR);

        // Match Python version FPS.
        SetTargetFPS(40);

        Slideshow slideshow(dbPhotos, screenWidth, screenHeight, config, database);

        while (slideshow.loopRunning()) {
            slideshow.prefetch();
            slideshow.move();
            // slideshow.fetch_twilio_photos();
            slideshow.draw(starTexture);
            slideshow.handleKeyboard();
        }

        CloseWindow();

        return 0;
    }
}

int main(int argc, char *argv[]) {
    try {
        return mainCanThrow(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Caught exception (" << e.what() << ")" << '\n';
    }
    return -1;
}
