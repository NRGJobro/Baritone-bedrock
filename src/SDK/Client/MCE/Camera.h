#pragma once

#include "../../Render/Matrix/MatrixStack.h"

namespace mce {
    class Camera {
    public:
    	MatrixStack viewMatrixStack;
    	MatrixStack worldMatrixStack;
    	MatrixStack projectionMatrixStack;
    	Matrix viewMatrix;
    	Matrix projectionMatrix;
    };
}
