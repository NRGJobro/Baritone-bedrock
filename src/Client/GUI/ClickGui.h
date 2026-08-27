#pragma once

#include "../../SDK/Client/MCE/Mesh.h"

class ClickGui {
public:
    static void render();
    static void onKey(int key, bool pressed, bool& cancel);
    static void onMouse(int button, bool pressed, bool& cancel);
    static void onWheel(bool direction, bool& cancel);

private:
    static inline bool builtMeshes = false;
    static inline mce::Mesh bgMesh{}, lineMesh{}, modMesh{}, settingsMesh{}, enabledStateMesh{}, circle{};

    static void buildMeshes();
};
