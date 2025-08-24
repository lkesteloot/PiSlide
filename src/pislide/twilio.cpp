
#include <iostream>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include "twilio.h"

namespace {
    static std::string URL_BASE = "https://api.twilio.com";

    // We get a 403 fetching images without this:
    static std::string USER_AGENT = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36";

    /**
     * The info we need out of each Twilio message.
     */
    struct TwilioMessage final {
        std::string uri;
        std::string sourcePhoneNumber;
        std::string dateSent;
        std::string mediaListUrl;
    };

    // Returns the JSON content of the specified Twilio-relative path (after the
    // domain name), or a null JSON object if an error occurred.
    nlohmann::json fetchJson(std::string const &path, Config const &config) {
        cpr::Response r = cpr::Get(
                cpr::Url{URL_BASE + path},
                cpr::Authentication{config.twilioSid, config.twilioToken, cpr::AuthMode::BASIC},
                cpr::Header{{"User-Agent", USER_AGENT}});
        if (r.status_code != 200) {
            std::cerr << "Got status code " << r.status_code
                << " fetching Twilio information at " << path << '\n';
            return nullptr;
        }

        return nlohmann::json::parse(r.text);
    }

    // Delete the specified resource (message or media). Returns whether successful.
    bool deleteResource(std::string const &path, Config const &config) {
        cpr::Response r = cpr::Delete(
                cpr::Url{URL_BASE + path},
                cpr::Authentication{config.twilioSid, config.twilioToken, cpr::AuthMode::BASIC},
                cpr::Header{{"User-Agent", USER_AGENT}});
        if (r.status_code != 200 && r.status_code != 204) {
            std::cerr << "Got status code " << r.status_code
                << " deleting Twilio resource at " << path << '\n';
            return false;
        }

        return true;
    }

    // Download and save the image at the specified Twilio-relative path (after
    // the domain name). Returns whether successful.
    bool downloadImage(std::string const &path, std::filesystem::path const &pathname) {
        std::cout << "Fetching Twilio photo to " << pathname << '\n';

        // Make sure the directory exists.
        std::filesystem::path dir = pathname.parent_path();
        bool createdDir = std::filesystem::create_directories(dir);
        if (createdDir) {
            std::cout << "Created Twilio download directory " << dir << '\n';
        }

        // Stream the file.
        std::ofstream f(pathname, std::ios::binary);
        cpr::Response r = cpr::Download(f,
                cpr::Url{URL_BASE + path},
                cpr::Header{{"User-Agent", USER_AGENT}});
        if (r.status_code != 200) {
            std::cerr << "Got status code " << r.status_code
                << " fetching Twilio image at " << path << '\n';
            return false;
        }
        return true;
    }

    // Return top-level message information, or an empty vector if it can't be fetched.
    std::vector<std::shared_ptr<TwilioMessage>> fetchMessages(Config const &config) {
        std::vector<std::shared_ptr<TwilioMessage>> messages;

        std::string path = "/2010-04-01/Accounts/" + config.twilioSid + "/Messages.json";
        auto j = fetchJson(path, config);
        if (j == nullptr) {
            // Error already printed.
            return messages;
        }

        for (auto const &jmsg : j["messages"]) {
            int numMedia = std::stoi(jmsg["num_media"].get<std::string>());
            if (numMedia > 0 && jmsg["direction"] == "inbound") {
                messages.emplace_back(std::make_shared<TwilioMessage>(TwilioMessage{
                    .uri = jmsg["uri"],
                    .sourcePhoneNumber = jmsg["from"],
                    .dateSent = jmsg["date_sent"],
                    .mediaListUrl = jmsg["subresource_uris"]["media"],
                }));
            }
        }

        return messages;
    }
}

std::vector<std::shared_ptr<TwilioImage>> downloadTwilioImages(
        bool deleteMessages, bool deleteImages,
        Config const &config) {

    std::vector<std::shared_ptr<TwilioImage>> images;

    std::cout << "Twilio: Fetching messages\n";
    auto messages = fetchMessages(config);
    for (auto message : messages) {
        std::cout << "Twilio: Processing message " << message->uri << '\n';
        bool allSuccess = true;
        if (!message->mediaListUrl.empty()) {
            auto medias = fetchJson(message->mediaListUrl, config);
            for (auto const &media : medias["media_list"]) {
                std::string contentType = media["content_type"];
                std::string photoSid = media["sid"];
                std::string mediaUri = media["uri"];
                if (contentType == "image/jpeg" && mediaUri.ends_with(".json")) {
                    std::string imageUri = mediaUri.substr(0, mediaUri.length() - 5);
                    std::filesystem::path imagePathname = config.twilioSubdir / (photoSid + ".jpg");
                    bool success = downloadImage(imageUri, config.rootDir / imagePathname);
                    if (success) {
                        images.emplace_back(std::make_shared<TwilioImage>(
                                    TwilioImage{
                                        .pathname = imagePathname,
                                        .contentType = contentType,
                                        .sourcePhoneNumber = message->sourcePhoneNumber,
                                        .dateSent = message->dateSent
                                    }));
                        if (deleteImages) {
                            deleteResource(mediaUri, config);
                        }
                    } else {
                        allSuccess = false;
                    }
                }
            }
        }
        if (deleteMessages && allSuccess) {
            deleteResource(message->uri, config);
        }
    }

    return images;
}

#ifdef TEST_TWILIO
int main(int argc, char *argv[]) {
    // As a test, this connects to the Twilio service, downloads all the
    // available photos, and optionally deletes all media and messages.
    /// bool delete = false;

    Config config;
    bool success = config.readConfigFile("config.toml");
    if (!success) {
        return 1;
    }
    success = config.parseCommandLine(argc, argv);
    if (!success || !config.isValid()) {
        return 1;
    }

    if (false) {
        auto messages = fetchMessages(config);
        for (auto message : messages) {
            std::cout << message->sourcePhoneNumber << " " << message->dateSent << " " << message->mediaListUrl << '\n';
        }
    }
    if (true) {
        auto images = downloadTwilioImages(false, true, config);
        for (auto image : images) {
            std::cout << image->sourcePhoneNumber << " " << image->dateSent << " " << image->pathname << '\n';
        }
    }
}
#endif
