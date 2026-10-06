#pragma once

#include "../SDK/Client/MCE/Color.h"

class Utils {
public:
    static uintptr_t findSig(const hat::signature_view& signature);

#ifndef NDEBUG
    static std::string convertSigViewToString(hat::signature_view const& sig);
#endif

    static const std::string& getLocalAppdataFolder();
    static const std::string& getClientFolder();

    static bool doesFolderExist(const std::string& path);

    static void createFolder(const std::string& path);

    static mce::Color HSVtoRGB(float hue, float saturation, float value);

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

};

template<>
uintptr_t Utils::getFromOffset<uintptr_t>(uintptr_t sig, int offset);
