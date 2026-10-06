#include "Utils.h"

#include "Logger.h"

uintptr_t Utils::findSig(const hat::signature_view& signature) {
    const auto result = find_pattern(signature, ".text");

    if (result.has_result())
        return reinterpret_cast<uintptr_t>(result.get());

#ifndef NDEBUG
    logF("Signature dead: {}", convertSigViewToString(signature));
#endif

    return 0;
}

#ifndef NDEBUG
std::string Utils::convertSigViewToString(hat::signature_view const &sig) {
    std::stringstream stream;

    for (auto& element : sig) {
        if (element.has_value())
            stream << fmt::format("{:02X}", static_cast<uint8_t>(element.value())) << " ";
        else
            stream << "? ";
    }

    std::string str = stream.str();
    str = str.substr(0, str.size() - 1);

    return str;
}
#endif

const std::string& Utils::getLocalAppdataFolder() {
    static std::string path;

    if (path.empty()) {
        char buf[MAX_PATH]{};
        const auto res = GetEnvironmentVariableA("LOCALAPPDATA", buf, MAX_PATH);

        if (res > 0)
            path = std::string(buf, res) + "\\";
    }

    return path;
}

const std::string& Utils::getClientFolder() {
    static auto path = getLocalAppdataFolder() + "Limiter\\";

    return path;
}

bool Utils::doesFolderExist(const std::string& path) {
    if (!std::filesystem::exists(path))
        return false;

    return std::filesystem::is_directory(path);
}

void Utils::createFolder(const std::string& path) {
    if (doesFolderExist(path))
        return;

    std::filesystem::create_directory(path);
}

mce::Color Utils::HSVtoRGB(float hue, const float saturation, const float value) {
    if (saturation == 0.f)
        return {value, value, value};

    hue = fmodf(hue, 1.f) / (60.f / 360.f);
    const int iHue = static_cast<int>(hue);
    const float f = hue - static_cast<float>(iHue);
    const float p = value * (1.f - saturation);
    const float q = value * (1.f - saturation * f);
    const float t = value * (1.f - saturation * (1.f - f));

    switch (iHue) {
        case 0:
            return {value, t, p};
        case 1:
            return {q, value, p};
        case 2:
            return {p, value, t};
        case 3:
            return {p, q, value};
        case 4:
            return {t, p, value};
        case 5:
        default:
            return {value, p, q};
    }
}

template<> uintptr_t Utils::getFromOffset<uintptr_t>(const uintptr_t sig, const int offset) {
    return sig + offset + 4 + *reinterpret_cast<int*>(sig + offset);
}
