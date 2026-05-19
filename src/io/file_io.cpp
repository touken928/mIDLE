#include "io/file_io.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace midle {

FileLoadResult load_file(const char *path) {
    FileLoadResult result;
    if (!path || !path[0]) {
        result.status = FileLoadStatus::NotFound;
        return result;
    }

    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        result.status = FileLoadStatus::NotFound;
        return result;
    }

    std::ifstream f(path, std::ios::binary);
    if (!f) {
        result.status = FileLoadStatus::IOError;
        return result;
    }

    std::ostringstream ss;
    ss << f.rdbuf();
    if (!f.good() && !f.eof()) {
        result.status = FileLoadStatus::IOError;
        return result;
    }

    result.status = FileLoadStatus::Ok;
    result.content = ss.str();
    return result;
}

void save_file(const std::string &path, const std::string &content) {
    std::filesystem::path p(path);
    auto parent = p.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
    }
    std::ofstream f(path);
    if (!f) return;
    f << content;
}

} // namespace midle
