#pragma once

namespace Core {
    enum class FileAccessType : int {
        ReadOnly,
        ReadWrite,
        Flush
    };
}
