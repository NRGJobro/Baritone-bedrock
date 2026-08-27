#pragma once

#include "File.h"
#include "FileOpenMode.h"

namespace Core {
    class FileStdStreamBuf : public std::streambuf {
        virtual void Destructor();

    public:
        File file;
        FileOpenMode fileOpenMode;
        std::vector<int8_t> buffer;
        uint64_t bufferSize;
    };
}
