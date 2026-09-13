#include "AppConfig.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

namespace pixel_world {
namespace {

std::filesystem::path executableDirectory() {
#ifdef _WIN32
    std::wstring modulePath(32768, L'\0');
    const DWORD length = GetModuleFileNameW(
        nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
    if (length > 0 && length < modulePath.size()) {
        modulePath.resize(length);
        return std::filesystem::path(modulePath).parent_path();
    }
#elif defined(__linux__)
    char modulePath[PATH_MAX] = {};
    const ssize_t length =
        readlink("/proc/self/exe", modulePath, sizeof(modulePath) - 1);
    if (length > 0) {
        modulePath[length] = '\0';
        return std::filesystem::path(modulePath).parent_path();
    }
#endif
    return {};
}

std::filesystem::path findConfigFile() {
    const std::filesystem::path currentDirectory =
        std::filesystem::current_path();
    const std::filesystem::path moduleDirectory = executableDirectory();

    std::vector<std::filesystem::path> roots{
        currentDirectory / "res",
    };
    if (!moduleDirectory.empty()) {
        roots.push_back(moduleDirectory / "res");
        roots.push_back(moduleDirectory / ".." / "res");
        roots.push_back(moduleDirectory / ".." / ".." / "res");
    }

    for (const std::filesystem::path& root : roots) {
        const std::filesystem::path candidate =
            (root / "config.ini").lexically_normal();
        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error)) {
            return candidate;
        }
    }
    return {};
}

std::string trim(std::string value) {
    const auto notSpace = [](unsigned char character) {
        return std::isspace(character) == 0;
    };
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), notSpace));
    value.erase(
        std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string lowercase(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

}  // namespace

Language AppConfig::loadLanguage() {
    const std::filesystem::path configFile = findConfigFile();
    if (configFile.empty()) {
        return Language::English;
    }

    std::ifstream input(configFile);
    std::string line;
    while (std::getline(input, line)) {
        const std::size_t commentPosition = line.find_first_of("#;");
        if (commentPosition != std::string::npos) {
            line.erase(commentPosition);
        }

        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = lowercase(trim(line.substr(0, separator)));
        const std::string value =
            lowercase(trim(line.substr(separator + 1)));
        if (key != "language") {
            continue;
        }
        if (value == "zh" || value == "cn" || value == "chinese") {
            return Language::Chinese;
        }
        if (value == "en" || value == "english") {
            return Language::English;
        }
    }

    return Language::English;
}

}  // namespace pixel_world
