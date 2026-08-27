#pragma once

namespace mce::framebuilder {
    struct RenderPlayerVisionDescription {
        bool nightVisionEnabled;
        float nightVisionScale;
        float blindnessLevel;
        float darknessLevel;
        float previousDarknessLevel;
    };
}
