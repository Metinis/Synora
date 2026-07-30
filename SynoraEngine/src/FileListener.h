#pragma once
#include "efsw/efsw.hpp"

class FileListener : public efsw::FileWatchListener {
    public:
    std::function<void(const std::string&, efsw::Action)> onFileChanged;

    void handleFileAction( efsw::WatchID watchid, const std::string& dir,
                           const std::string& filename, efsw::Action action,
                           const std::string& oldFilename ) override {
        if (dir.contains("/.hotreload/")) {
            return;
        }
        if (onFileChanged) {
            onFileChanged(dir + filename, action);
        }
    }
};

namespace efsw {
inline const char* actionToString(efsw::Action action){
    switch (action) {
    case efsw::Actions::Add:      return "Add";
    case efsw::Actions::Delete:   return "Delete";
    case efsw::Actions::Modified: return "Modified";
    case efsw::Actions::Moved:    return "Moved";
    default:                      return "Unknown";
    }
}
}
