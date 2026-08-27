#pragma once

struct WeatherRenderObject {
    struct WeatherParameters {
        float speed;
        float length;
        float width;
        float alpha;
        float windScale;
        bool normalizeWind;
        float gravityScale;
        float randomDirScale;
        float despawnProbability;
    };

    struct RuntimeValues {
        float speedScale;
        float windScale;
        glm::vec3 wind;
        glm::vec3 offset;
        glm::vec3 random;
        glm::vec3 snowOffset;
        glm::vec3 randomDir;
        int particleIndexOffset;
    };

    struct RenderRuntimeSettings {
        RuntimeValues t0;
        RuntimeValues t;
    };

    enum PrecipitationType : int {
        PrecipitationRain,
        PrecipitationSnow,
        PrecipitationPlankton,
        PrecipitationRedSpores,
        PrecipitationWhiteSpores,
        PrecipitationAsh,
        PrecipitationWhiteAsh,
        PrecipitationCount
    };

    std::shared_ptr<mce::Mesh> particleMesh;
    mce::TexturePtr weatherTexture;
    mce::ServerTexture occlusionTexture;
    mce::ServerTexture brightnessTexture;
    mce::ServerTexture unkTexture;
    uint64_t unk;
    mce::MaterialPtr* rainMat;
    mce::MaterialPtr* snowMat;
    mce::MaterialPtr* planktonMat;
    mce::MaterialPtr* redSporesMat;
    mce::MaterialPtr* blueSporesMat;
    mce::MaterialPtr* ashMat;
    mce::MaterialPtr* whiteAshMat;
    std::array<WeatherParameters, 7> params;
    std::array<std::array<RenderRuntimeSettings, 10>, 7> settings;
    float densityRain;
    float densitySnow;
    float densityPlankton;
    float densityRedSpores;
    float densityBlueSpores;
    float densityAsh;
    float densityWhiteAsh;
    float particleBoxSize;
    float luminance;
    int particleCount;
};
