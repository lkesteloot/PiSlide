
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
#include <fstream>
#include <sstream>
#include <chrono>

#include "raylib.h"
#include "TinyEXIF.h"

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
#include "util.h"
#include "label.h"

namespace {
    constexpr int MAX_FILE_WARNING_COUNT = 10;

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
    std::set<std::filesystem::path> traverseDirectoryTree(Config const &config) {
        auto rootDir = config.rootDir;
        std::set<std::filesystem::path> pathnames;

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
                    if (config.unwantedDirs.contains(path.filename())) {
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
     * Get the modified time of a file. The first in the pair is a string like "January 4, 2009".
     * The second is the number of seconds since the epoch.
     */
    std::pair<std::string,int64_t> getFileDate(std::filesystem::path const &pathname) {
        // Unreal.
        std::filesystem::file_time_type time1 = std::filesystem::last_write_time(pathname);
        auto time2 = time1 - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now();
        auto time3 = std::chrono::time_point_cast<std::chrono::system_clock::duration>(time2);
        std::time_t time4 = std::chrono::system_clock::to_time_t(time3);
        std::tm time5 = *std::localtime(&time4);

        std::ostringstream oss;
        oss << std::put_time(&time5, "%B %d, %Y");
        auto time6 = oss.str();

        // Remove leading 0 from day:
        auto space = time6.find(' ');
        if (space != std::string::npos && time6[space + 1] == '0') {
            time6.erase(space + 1, 1);
        }

        // Now for the epoch time.
        auto time7 = std::chrono::file_clock::to_sys(time1);
        auto time8 = time7.time_since_epoch();
        auto time9 = std::chrono::duration_cast<std::chrono::seconds>(time8).count();

        return std::make_pair(time6, time9);
    }

    /**
     * Return the file's rotation in degrees, or 0 if it cannot be determined.
     * The rotation specifies how much to rotate the loaded image counter-clockwise
     * in order to display it.
     */
    int getFileRotation(std::filesystem::path const &pathname) {
        int rotation;

        // Open a stream to read just the necessary parts of the image file.
	std::ifstream istream(pathname, std::ifstream::binary);

	// Parse image EXIF.
	TinyEXIF::EXIFInfo exif(istream);
        switch (exif.Fields ? exif.Orientation : 0) {
                                            // start of data is:
            case 0: rotation = 0; break;    //   unspecified in EXIF data
            case 1: rotation = 0; break;    //   upper left of image
            case 3: rotation = 180; break;  //   lower right of image
            case 6: rotation = -90; break;  //   upper right of image
            case 8: rotation = 90; break;   //   lower left of image
            case 9: rotation = 0; break;    //   undefined
            default: rotation = 0; break;
        }

        return rotation;
    }

    /**
     * Create a new photo file for this pathname, and optionally a new photo.
     * Returns the photo ID (new or old).
     */
    int32_t handleNewAndRenamedFile(Database const &database,
            Config const &config,
            std::filesystem::path const &pathname) {

        std::cout << "    Computing hash for " << pathname << '\n';
        std::string label = pathnameToLabel(config, pathname);

        // We want the hashes of the original files, not the processed ones.
        std::filesystem::path absolutePathname = config.rootDir / pathname;

        std::vector<std::byte> imageBytes = readFileBytes(absolutePathname);

        std::string hashAll = sha1Hex(imageBytes.data(), imageBytes.size());
        int start = std::max(0, (int) imageBytes.size() - 1024);
        std::string hashBack = sha1Hex(imageBytes.data() + start, imageBytes.size() - start);

        // Create a new photo file.
        PhotoFile photoFile { pathname, hashAll, hashBack };
        database.savePhotoFile(photoFile);

        // Now see if this was a renaming of another file.
        std::optional<Photo> photo = database.getPhotoByHashBack(hashBack);
        int32_t photoId;
        if (photo) {
            // Renamed or moved photo.
            std::cout << "        Renamed or moved photo.\n";
            // Leave the timestamp the same, but update the label.
            photo->label = label;
            database.savePhoto(*photo);
            photoId = photo->id;
        } else {
            // New photo.
            std::cout << "        New photo.\n";
            int rotation = getFileRotation(absolutePathname);
            std::pair<std::string,int64_t> fileDate = getFileDate(absolutePathname);
            std::cout << "            Rotation " << rotation << '\n';
            std::cout << "            Date " << fileDate.first << '\n';
            Photo newPhoto {
                .hashBack = hashBack,
                .rotation = rotation,
                .rating = 3,
                .date = fileDate.second,
                .displayDate = fileDate.first,
                .label = label,
            };

            photoId = database.insertPhoto(newPhoto);
        }
        std::cout << "            ID = " << photoId << "\n";

        return photoId;
    }

    /**
     * Look for new images or images that might have been moved or renamed in the tree.
     * We must make sure to not lose the metadata (rating, rotation).
     */
    void handleNewAndRenamedFiles(Database const &database,
            Config const &config,
            std::set<std::filesystem::path> const diskPathnames,
            std::vector<PhotoFile> const &dbPhotoFiles) {

        // Figure out which pathnames we need to analyze.
        std::set<std::filesystem::path> pathnamesToDo = diskPathnames;
        for (PhotoFile const &photoFile : dbPhotoFiles) {
            pathnamesToDo.erase(photoFile.pathname);
        }

        std::cout << "Analyzing " << pathnamesToDo.size() << " new or renamed photos...\n";
        for (std::filesystem::path const &pathname : pathnamesToDo) {
            handleNewAndRenamedFile(database, config, pathname);
        }
    }

    // Keep photos of at least this rating.
    void filterPhotosByRating(std::vector<Photo> &dbPhotos, Config const &config) {
        int minRating = config.minRating;

        dbPhotos.erase(std::remove_if(dbPhotos.begin(), dbPhotos.end(),
                    [minRating](Photo const &photo) {
                        return photo.rating < minRating;
                    }), dbPhotos.end());
    }

    /**
     * Keep photos in date range.
     */
    void filterPhotosByDate(std::vector<Photo> &dbPhotos, Config const &config) {
        time_t now = nowEpoch();
        long maxDate = config.minDays == 0 ? 0 : now - config.minDays*24*60*60;
        long minDate = config.maxDays == 0 ? 0 : now - config.maxDays*24*60*60;

        dbPhotos.erase(std::remove_if(dbPhotos.begin(), dbPhotos.end(),
                    [minDate, maxDate](Photo const &photo) {
                        return (minDate != 0 && photo.date < minDate) ||
                            (maxDate != 0 && photo.date > maxDate);
                    }), dbPhotos.end());
    }

    /**
     * Assign a pathname to each photo, returning a new copy of dbPhotos with
     * invalid photos (those with no disk files) removed.
     */
    std::vector<Photo> assignPhotoPathnames(Database const &database,
            Config const &config,
            std::vector<Photo> const &dbPhotos,
            std::set<std::filesystem::path> const &diskPathnames) {

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
            if (true || photo.pathname.string().find("P101082") != std::string::npos) {
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
        std::set<std::filesystem::path> diskPathnames = traverseDirectoryTree(config);
        std::cout << "Photos on disk: " << diskPathnames.size() << '\n';

        // TODO optionally resize images.

        // Get all photo files from the database.
        std::vector<PhotoFile> dbPhotoFiles = database.getAllPhotoFiles();
        std::cout << "Photo files in database: " << dbPhotoFiles.size() << '\n';

        // Compute missing hashes of photos, add them to database.
        handleNewAndRenamedFiles(database, config, diskPathnames, dbPhotoFiles);

        // Keep only photos of the right rating and date range.
        std::vector<Photo> dbPhotos = database.getAllPhotos();
        std::cout << "Photos in database: " << dbPhotos.size() << '\n';
        filterPhotosByRating(dbPhotos, config);
        std::cout << "Photos after rating filter: " << dbPhotos.size() << '\n';
        filterPhotosByDate(dbPhotos, config);
        std::cout << "Photos after date filter: " << dbPhotos.size() << '\n';

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

        // Open the display.
        InitWindow(config.windowWidth, config.windowHeight, "PiSlide");

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
