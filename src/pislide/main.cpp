
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

#include <raylib.h>
#include <TinyEXIF.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/std.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "database.h"
#include "slideshow.h"
#include "executor.h"
#include "slidecache.h"
#include "config.h"
#include "util.h"
#include "label.h"
#include "twiliofetcher.h"
#include "constants.h"
#include "webserver.h"

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
                    spdlog::error("Unknown directory entry type: {}", path);
                    pathnames.clear();
                    return pathnames;
                }
            }
        } catch (std::filesystem::filesystem_error const &e) {
            spdlog::error("Filesystem error: {}", e.what());
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

        spdlog::info("    Computing hash for {}", pathname);
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
            spdlog::info("        Renamed or moved photo");
            // Leave the timestamp the same, but update the label.
            photo->label = label;
            database.savePhoto(*photo);
            photoId = photo->id;
        } else {
            // New photo.
            spdlog::info("        New photo");
            int rotation = getFileRotation(absolutePathname);
            std::pair<std::string,int64_t> fileDate = getFileDate(absolutePathname);
            spdlog::info("            Rotation {}", rotation);
            spdlog::info("            Date {}", fileDate.first);
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
        spdlog::info("            ID = {}", photoId);

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

        spdlog::info("Analyzing {} new or renamed photos...", pathnamesToDo.size());
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
                    spdlog::info("No file on disk for {} ({})", photo.hashBack, photo.label);
                }
            }
        }

        if (warningCount != 0) {
            spdlog::info("Files missing on disk: {}", warningCount);
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
     * Temporary filter for just one event.
     */
    std::vector<Photo> filterPhotosForEvent(std::vector<Photo> const &dbPhotos) {
        // Max two years.
        long twoYears = nowEpoch() - 365*2*24*60*60;

        std::vector<Photo> goodPhotos;

        for (auto &photo : dbPhotos) {
            std::string pathname = photo.pathname.string();
            bool eventName = pathname.find("Event Name") != std::string::npos;
            bool badDir = pathname.find("Bad Dir") != std::string::npos;
            bool recent = photo.date > twoYears;

            if (eventName || (recent && !badDir)) {
                goodPhotos.push_back(photo);
            }
        }

        return goodPhotos;
    }

    /**
     * Guesses a file extension for the given content type.
     */
    std::string guessExtensionForContentType(std::string const &contentType) {
        if (contentType == "image/jpeg") {
            return ".jpg";
        }
        if (contentType == "image/png") {
            return ".png";
        }
        if (contentType == "image/webp") {
            return ".webp";
        }
        if (contentType == "image/gif") {
            return ".gif";
        }
        return ".bin";
    }

    /**
     * Returns a filename (with the given prefix and suffix) for a file in the
     * specified directory that's guaranteed to not refer to an existing file.
     */
    std::filesystem::path newFilenameInDirectory(std::filesystem::path const &absoluteDir,
            std::string const &prefix, std::string const &suffix) {

        int counter = 1;

        // Okay this sounds slow but really we only have a few dozen files in here.
        while (true) {
            std::filesystem::path filename = fmt::format("{}{:08}{}", prefix, counter, suffix);
            std::filesystem::path absolutePathname = absoluteDir / filename;
            if (!std::filesystem::exists(absolutePathname)) {
                return filename;
            }
            counter += 1;
        }
    }

    void fetchTwilioImages(TwilioFetcher &twilioFetcher,
            Database const &database,
            Config const &config,
            Slideshow &slideshow) {

        // Initiate a Twilio fetch if we're in party mode.
        if (slideshow.isParty()) {
            twilioFetcher.initiateFetch(MIN_TWILIO_FETCH_INTERVAL_S);
        }

        // See if any images came in from the Twilio thread. If so, process them
        // and show them next. Do this regardless of party mode in order to catch
        // images that were fetched but not yet retrieved when party mode was
        // turned off.
        std::vector<std::shared_ptr<TwilioImage>> images = twilioFetcher.get();
        for (auto image : images) {
            // Compute hash, add to database. This takes a bit of time and
            // causes a hiccup in the animation. We'd have to split the hash
            // and the database work to avoid that.
            int32_t photoId = handleNewAndRenamedFile(database, config, image->pathname);

            // Fetch back from database and fix up pathnames.
            std::optional<Photo> photo = database.getPhotoById(photoId);
            if (photo) {
                photo->pathname = image->pathname;
                photo->absolutePathname = config.rootDir / photo->pathname;
                slideshow.insertPhoto(*photo);
            } else {
                spdlog::error("Didn't find expected photo {}", photoId);
            }
        }
    }

    void fetchWebUploadImages(ThreadSafeQueue<WebUpload> &queue,
            Database const &database,
            Config const &config,
            Slideshow &slideshow) {

        while (true) {
            // See if any images came in from the web thread. If so, process them
            // and show them next.
            auto upload = queue.try_dequeue();
            if (!upload) {
                break;
            }

            // Determine extension.
            std::string extension = guessExtensionForContentType(upload->contentType);

            // Come up with pathname.
            std::filesystem::path absoluteDir = config.rootDir / config.webSubdir;
            std::filesystem::path filename = newFilenameInDirectory(absoluteDir,
                    "web-upload-", extension);
            std::filesystem::path pathname = config.webSubdir / filename;
            std::filesystem::path absolutePathname = config.rootDir / pathname;

            // Create the directory if necessary.
            bool createdDir = std::filesystem::create_directories(absoluteDir);
            if (createdDir) {
                spdlog::info("Created web upload directory {}", absoluteDir);
            }

            // Write the file to disk.
            spdlog::info("Saving web file to {}", pathname);
            std::ofstream ofs(absolutePathname, std::ios::binary);
            ofs << upload->content;
            ofs.close();

            // Compute hash, add to database. This takes a bit of time and
            // causes a hiccup in the animation. We'd have to split the hash
            // and the database work to avoid that.
            int32_t photoId = handleNewAndRenamedFile(database, config, pathname);

            // Fetch back from database and fix up pathnames.
            std::optional<Photo> photo = database.getPhotoById(photoId);
            if (photo) {
                photo->pathname = pathname;
                photo->absolutePathname = absolutePathname;
                slideshow.insertPhoto(*photo);
            } else {
                spdlog::error("Didn't find expected photo {}", photoId);
            }
        }
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

        // Configure logging.
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "logs/pislide", 1024*1024*10, 10);
        auto ringBufferSink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(DEBUG_LOG_COUNT);
        auto logger = std::make_shared<spdlog::logger>(
                "logger", spdlog::sinks_init_list({consoleSink, fileSink, ringBufferSink}));
        logger->flush_on(spdlog::level::trace);
        logger->set_level(spdlog::level::debug);
        spdlog::set_default_logger(logger);

        spdlog::info("--- PiSlide ---");

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

        // Start the web server for uploading photos.
        ThreadSafeQueue<WebUpload> webUploadQueue;
        std::unique_ptr<WebServer> webServer = startWebServer(config, webUploadQueue);

        // TODO upgrade the schema.

        // Recursively read the photo tree from the disk.
        std::set<std::filesystem::path> diskPathnames = traverseDirectoryTree(config);
        spdlog::info("Photos on disk: {:L}", diskPathnames.size());

        // Get all photo files from the database.
        std::vector<PhotoFile> dbPhotoFiles = database.getAllPhotoFiles();
        spdlog::info("Photo files in database: {}", dbPhotoFiles.size());

        // Compute missing hashes of photos, add them to database.
        handleNewAndRenamedFiles(database, config, diskPathnames, dbPhotoFiles);

        // Keep only photos of the right rating and date range.
        std::vector<Photo> dbPhotos = database.getAllPhotos();
        spdlog::info("Photos in database: {}", dbPhotos.size());
        filterPhotosByRating(dbPhotos, config);
        spdlog::info("Photos after rating filter: {}", dbPhotos.size());
        filterPhotosByDate(dbPhotos, config);
        spdlog::info("Photos after date filter: {}", dbPhotos.size());

        // Find a pathname for each photo.
        dbPhotos = assignPhotoPathnames(database, config, dbPhotos, diskPathnames);
        spdlog::info("Photos found on disk: {}", dbPhotos.size());

        dbPhotos = filterPhotosByPathnameSubstring(dbPhotos);
        spdlog::info("Photos after pathname filter: {}", dbPhotos.size());

        // dbPhotos = filterPhotosForEvent(dbPhotos);
        // spdlog::info("Photos after event filter: {}", dbPhotos.size());

        spdlog::info("Final photos to be shown: {}", dbPhotos.size());

        if (dbPhotos.empty()) {
            spdlog::error("No photos found");
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
        spdlog::info("Window size: {}x{}", screenWidth, screenHeight);

        // Load the star icon.
        Texture starTexture = LoadTexture("src/resources/outline-star-256.png");
        if (!IsTextureValid(starTexture)) {
            spdlog::error("Can't load star texture");
        }
        GenTextureMipmaps(&starTexture);
        SetTextureFilter(starTexture, TEXTURE_FILTER_TRILINEAR);

        // Create the party mode QR code.
        std::optional<qrcodegen::QrCode> qrCode;
        if (!config.partyQrCode.empty()) {
            qrCode = qrcodegen::QrCode::encodeText(
                    config.partyQrCode.c_str(), qrcodegen::QrCode::Ecc::LOW);
        }

        // Match Python version FPS.
        SetTargetFPS(40);

        // Fetch Twilio images in another thread.
        TwilioFetcher twilioFetcher(config);
        // We've seen cases where messages appeared in the list a few
        // seconds before their photos were available. If we catch it in
        // that window, we'll delete the message before we have a chance
        // to get the photo. So delete messages, just let them live
        // indefinitely. If that becomes a problem, we can delete messages
        // older than a certain age.
        twilioFetcher.setDeleteMessages(false);
        twilioFetcher.setDeleteImages(true);

        {
            // Nested scope to delete slideshow before we close the window.
            Slideshow slideshow(dbPhotos, screenWidth, screenHeight, config, database,
                    ringBufferSink);

            while (slideshow.loopRunning()) {
                fetchTwilioImages(twilioFetcher, database, config, slideshow);
                fetchWebUploadImages(webUploadQueue, database, config, slideshow);
                slideshow.prefetch();
                slideshow.move();
                slideshow.draw(starTexture, qrCode);
                slideshow.handleKeyboard();
            }
        }

        CloseWindow();

        return 0;
    }
}

int main(int argc, char *argv[]) {
    try {
        return mainCanThrow(argc, argv);
    } catch (const std::exception &e) {
        spdlog::error("Caught exception ({})", e.what());
    }
    return -1;
}
