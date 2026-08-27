#pragma once

#include "../SDK/Client/MCE/Color.h"

#define FindSig(sig) Utils::findSigLegacy(sig)

class Utils {
public:
    static uintptr_t findSigLegacy(const std::string& sig);
    static uintptr_t findSig(const hat::signature_view& signature);

#ifndef NDEBUG
    static std::string convertSigViewToString(hat::signature_view const& sig);
#endif

    static std::vector<std::string> splitString(const std::string& str, char delimiter);

    static const std::string& getLocalAppdataFolder();
    static const std::string& getClientFolder();

    static bool doesFileExist(const std::string& path);
    static bool doesFolderExist(const std::string& path);

    static void createFolder(const std::string& path);

    static void writeFile(const std::string& path, const std::vector<std::string>& data);
    static std::vector<std::string> readFile(const std::string& path);

    static mce::Color HSVtoRGB(float hue, float saturation, float value);
    static glm::vec3 RGBtoHSV(float red, float green, float blue);

    static std::string toHexAddress(const void* ptr);

    static void patchBytes(void* dst, const void* src, uint32_t size);

    template <uint32_t FuncIndex, typename ReturnType, typename... ArgsTypes>
    static auto CallVFunc(auto* instancePtr, ArgsTypes... argsList) -> ReturnType {
        using InstanceType = std::remove_pointer_t<decltype(instancePtr)>;
        using FunctionType = ReturnType(InstanceType::*)(ArgsTypes...);

        void* voidPtr = (*reinterpret_cast<void***>(instancePtr))[FuncIndex];
        auto functionPtr = std::bit_cast<FunctionType>(voidPtr);

        return (instancePtr->*functionPtr)(argsList...);
    }

    template <typename ret>
    static ret getFromOffset(const uintptr_t sig, const int offset) {
        return reinterpret_cast<ret>(sig + offset + 4 + *reinterpret_cast<int*>(sig + offset));
    }

    template<>
    static uintptr_t getFromOffset(uintptr_t sig, int offset);

    static bool isLost(HRESULT res);

    static bool isUsingDarkTheme();
};
