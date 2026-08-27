#pragma once

enum class ResourceFileSystem : int {
    UserPackage,
    AppPackage,
    Raw,
    RawPersistent,
    SettingsDir,
    ExternalDir,
    ServerPackage,
    DataDir,
    UserDir,
    WorldDir,
    StoreCache,
    MaterialsDir,
    ShadersSDKDir,
    Invalid
};
