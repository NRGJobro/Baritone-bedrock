#pragma once

#include "FileStream.h"

namespace Core {
    class OutputFileStream : public FileStream, public virtual std::ios {
        virtual void Destructor();
    };
}
