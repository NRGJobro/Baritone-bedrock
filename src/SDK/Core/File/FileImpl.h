#pragma once
#include "FileOpenMode.h"

namespace Core {
    class FileImpl {
        void** vtable;

    public:
        FileOpenMode openMode;
        class FileSystemImpl* transaction;
        void* unk;
        bool loggingEnabled;
    };
}
