#pragma once

namespace Core {
    struct FileOpenMode {
        bool read : 1;
        bool write : 1;
        bool create : 1;
        bool truncate : 1;
        bool append : 1;
        bool binary : 1;
        bool unknown : 1;
    };
}
