#pragma once

#include "FileImpl.h"

namespace Core {
    class File {
    public:
        std::unique_ptr<FileImpl> file;
        std::unique_ptr<FileSystemImpl> transaction;
    };
}
