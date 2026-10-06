#pragma once

#include "../../SDK/Client/MCE/Mesh.h"

class ClickGui {
public:
    static void setOpen(bool open);
    static void maintainMouseCapture();
    static void render();
    static void onKey(int key, bool pressed, bool& cancel);
    static void onMouse(int button, bool pressed, bool& cancel);
    static void onWheel(bool direction, bool& cancel);

private:
    static inline bool builtMeshes = false;
    static inline mce::Mesh overlayMesh{}, shellMesh{}, headerMesh{}, sidebarMesh{};
    static inline mce::Mesh redlineMesh{}, gaugeMesh{}, cardMesh{}, cardHoverMesh{}, cardActiveMesh{};
    static inline mce::Mesh settingsMesh{}, toggleTrackMesh{}, knobMesh{};

    static void buildMeshes();
};
