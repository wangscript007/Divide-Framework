#include "stdafx.h"

#include "Core/Headers/StringHelper.h"
#include "Headers/FileManagement.h"

#include <filesystem>

namespace Divide {

bool writeFile(const char* filePath, const char* fileName, const bufferPtr content, const size_t length, const FileType fileType) {

    if (!Util::IsEmptyOrNull(filePath) && content != nullptr && length > 0) {
        if (!pathExists(filePath) && !CreateDirectories(filePath)) {
            return false;
        }

        std::ofstream outputFile(stringImpl{ filePath } +fileName,
                                 fileType == FileType::BINARY
                                           ? std::ios::out | std::ios::binary
                                           : std::ios::out);

        outputFile.write(static_cast<char*>(content), length);
        outputFile.close();
        return outputFile.good();
    }

    return false;
}

stringImpl stripQuotes(const char* input) {
    stringImpl ret = input;

    if (!ret.empty()) {
        ret.erase(std::remove(std::begin(ret), std::end(ret), '\"'), std::end(ret));
    }

    return ret;
}

FileWithPath splitPathToNameAndLocation(const char* input) {
    FileWithPath ret;
    ret._path = input;

    size_t pathNameSplitPoint = ret._path.find_last_of('/') + 1;
    if (pathNameSplitPoint == 0) {
        pathNameSplitPoint = ret._path.find_last_of('\\') + 1;
    }

    ret._fileName = ret._path.substr(pathNameSplitPoint, stringImpl::npos),
    ret._path = ret._path.substr(0, pathNameSplitPoint);
    return ret;
}

bool pathExists(const char* filePath) {
    const auto targetPath = std::filesystem::path(filePath);
    return std::filesystem::is_directory(targetPath);
}

bool createDirectory(const char* path) {
    if (!pathExists(path)) {
        const auto targetPath = std::filesystem::path(path);
        std::error_code ec = {};
        return std::filesystem::create_directory(targetPath, ec);
    }

    return true;
}

bool fileExists(const char* filePathAndName) {
    const auto targetPath = std::filesystem::path(filePathAndName);
    return std::filesystem::is_regular_file(targetPath);
}

bool fileExists(const char* filePath, const char* fileName) {
    const auto targetPath = std::filesystem::path(stringImpl{ filePath } +fileName);
    return std::filesystem::is_regular_file(targetPath);
}

bool createFile(const char* filePathAndName, bool overwriteExisting) {
    if (overwriteExisting && fileExists(filePathAndName)) {
        return std::ofstream(filePathAndName, std::fstream::in | std::fstream::trunc).good();
    }

    CreateDirectories((const_sysInfo()._pathAndFilename._path + "/" + splitPathToNameAndLocation(filePathAndName)._path).c_str());

    return std::ifstream(filePathAndName, std::fstream::in).good();
}

bool openFile(const char* filePath, const char* fileName) {
    return openFile("", filePath, fileName);
}

bool openFile(const char* cmd, const char* filePath, const char* fileName) {
    if (Util::IsEmptyOrNull(fileName) || !fileExists(filePath, fileName)) {
        return false;
    }

    constexpr std::array<std::string_view, 2> searchPattern = {
        "//", "\\"
    };

    const stringImpl file = "\"" + Util::ReplaceString({ const_sysInfo()._pathAndFilename._path + "/" + filePath + fileName }, searchPattern, "/", true) + "\"";

    if (strlen(cmd) == 0) {
        return CallSystemCmd(file.c_str(), "");
    }

    return CallSystemCmd(cmd, file.c_str());
}

bool deleteFile(const char* filePath, const char* fileName) {
    if (Util::IsEmptyOrNull(fileName)) {
        return false;
    }
    const std::filesystem::path file(stringImpl{ filePath } +fileName);
    std::filesystem::remove(file);
    return true;
}

bool copyFile(const char* sourcePath, const char* sourceName, const char* targetPath, const char* targetName, bool overwrite) {
    if (Util::IsEmptyOrNull(sourceName) || Util::IsEmptyOrNull(targetName)) {
        return false;
    }
    stringImpl source{ sourcePath };
    source.append(sourceName);

    if (!fileExists(source.c_str())) {
        return false;
    }

    stringImpl target{ targetPath };
    target.append(targetName);

    if (!overwrite && fileExists(target.c_str())) {
        return false;
    }

    std::filesystem::copy_file(std::filesystem::path(source),
                               std::filesystem::path(target),
                               std::filesystem::copy_options::overwrite_existing);

    return true;
}

bool findFile(const char* filePath, const char* fileName, stringImpl& foundPath) {
    const std::filesystem::path dir_path(filePath);
    std::filesystem::path file_name(fileName);

    const std::filesystem::recursive_directory_iterator end;
    const auto it = std::find_if(std::filesystem::recursive_directory_iterator(dir_path), end,
        [&file_name](const std::filesystem::directory_entry& e) {
        return e.path().filename() == file_name;
    });
    if (it == end) {
        return false;
    } else {
        foundPath = it->path().string();
        return true;
    }
}

bool hasExtension(const char* filePath, const Str16& extension) {
    const Str16 ext("." + extension);
    return Util::CompareIgnoreCase(Util::GetTrailingCharacters(stringImplFast{ filePath }, ext.length()), ext);
}

bool deleteAllFiles(const char* filePath, const char* extension) {
    bool ret = false;

    if (pathExists(filePath)) {
        const std::filesystem::path pathIn(filePath);
        for (const auto& p : std::filesystem::directory_iterator(pathIn)) {
            try {
                if (std::filesystem::is_regular_file(p.status())) {
                    if (!extension || (extension != nullptr && p.path().extension() == extension)) {
                        if (std::filesystem::remove(p.path())) {
                            ret = true;
                        }
                    }
                } else {
                    //ToDo: check if this recurse in subfolders actually works
                    deleteAllFiles(p.path().string().c_str(), extension);
                }
            } catch (const std::exception &ex) {
                ACKNOWLEDGE_UNUSED(ex);
            }
        }
    }

    return ret;
}

bool clearCache() {
    for (U8 i = 0; i < to_U8(CacheType::COUNT); ++i) {
        if (!clearCache(static_cast<CacheType>(i))) {
            return false;
        }
    }
    return true;
}

bool clearCache(const CacheType type) {
    switch (type) {
        case CacheType::SHADER_TEXT:
           return deleteAllFiles(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationText.c_str());
       
        case CacheType::SHADER_BIN:
            return deleteAllFiles(Paths::g_cacheLocation + Paths::Shaders::g_cacheLocationBin.c_str());

        case CacheType::TERRAIN:
            return deleteAllFiles(Paths::g_cacheLocation + Paths::g_terrainCacheLocation.c_str());

        case CacheType::MODELS:
            return deleteAllFiles(Paths::g_cacheLocation + Paths::g_geometryCacheLocation.c_str());

        case CacheType::TEXTURES:
            return deleteAllFiles(Paths::g_cacheLocation + Paths::Textures::g_metadataLocation.c_str());

       default: break;
    }

    return false;
}

std::string extractFilePathAndName(char* argv0) {
    std::error_code ec;
    auto currentPath = std::filesystem::current_path();
    currentPath.append(argv0);
    std::filesystem::path p(std::filesystem::canonical(currentPath, ec));
    return p.make_preferred().string();
}

}; //namespace Divide
