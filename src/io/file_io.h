#pragma once

#include <string>

namespace midle {

enum class FileLoadStatus {
    Ok,         // file read (content may be empty)
    NotFound,   // path does not exist
    IOError,    // exists but could not be read
};

struct FileLoadResult {
    FileLoadStatus status = FileLoadStatus::IOError;
    std::string content;
};

// Load entire file. Distinguishes missing files from empty files.
FileLoadResult load_file(const char *path);

void save_file(const std::string &path, const std::string &content);

} // namespace midle
