#pragma once

namespace SYN {
    //keep info of current active project stuff, e.g. resource dir, asset manager
    //differs from application which is the entire engine, this is the active game project being worked on
    //e.g. when switching out a game, we want new asset manager etc
    struct ProjectConfig {
        std::string projectRoot{};
        std::string resourceRoot{};
    };
}
