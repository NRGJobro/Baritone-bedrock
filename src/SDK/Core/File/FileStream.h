#pragma once

#include "FileStdStreamBuf.h"

namespace Core {
    class FileStream : public std::iostream, public virtual std::ios {
        virtual void Destructor();

    public:
        FileStdStreamBuf streamBuffer;
        bool loggingEnabled;
    };
}
