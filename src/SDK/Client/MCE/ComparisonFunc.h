#pragma once

namespace mce {
    enum ComparisonFunc : uint8_t {
        Equal,
        NotEqual,
        Always,
        Less,
        Greater,
        GreaterEqual,
        LessEqual,
        Never
    };
}
