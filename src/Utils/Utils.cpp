#include "Utils.h"

#include "Logger.h"

uintptr_t Utils::findSigLegacy(const std::string& sig) {
    return findSig(hat::parse_signature(sig).value());
}

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

std::vector<std::string> Utils::splitString(const std::string& str, char delimiter) {
    return str
          | std::views::split(delimiter)
          | std::views::transform([](auto&& token) { return std::string{token.begin(), token.end()}; })
          | std::ranges::to<std::vector>();
}

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

bool Utils::doesFileExist(const std::string& path) {
    if (!std::filesystem::exists(path))
        return false;

    return std::filesystem::is_regular_file(path);
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

void Utils::writeFile(const std::string& path, const std::vector<std::string>& data) {
    std::ofstream file(path, std::ios::out | std::ios::trunc);

    if (!file.is_open())
        return;

    for (const auto& str : data) {
        file << str << std::endl;
    }

    file.close();
}

std::vector<std::string> Utils::readFile(const std::string& path) {
    if (!doesFileExist(path))
        return {};

    std::ifstream file;
    file.open(path);

    if (!file.is_open())
        return {};

    std::string line;
    std::vector<std::string> result;

    while (std::getline(file, line)) {
        result.push_back(line);
    }

    file.close();

    return result;
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

glm::vec3 Utils::RGBtoHSV(float red, float green, float blue) {
    float K = 0.f;

    if (green < blue) {
        {
            const float tmp = green;
            green = blue;
            blue = tmp;
        }

        K = -1.f;
    }

    if (red < green) {
        {
            const float tmp = red;
            red = green;
            green = tmp;
        }

        K = -2.f / 6.f - K;
    }

    const float chroma = red - (green < blue ? green : blue);

    return {fabsf(K + (green - blue) / (6.f * chroma + 1e-20f)), chroma / (red + 1e-20f), red};
}

std::string Utils::toHexAddress(const void* ptr) {
    return fmt::format("0x{:X}", reinterpret_cast<uintptr_t>(ptr));
}

void Utils::patchBytes(void* dst, const void* src, const uint32_t size) {
    if (src == nullptr || dst == nullptr)
        return;

    DWORD oldprotect;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy(dst, src, size);
    VirtualProtect(dst, size, oldprotect, &oldprotect);
}

template<> uintptr_t Utils::getFromOffset(const uintptr_t sig, const int offset) {
    return sig + offset + 4 + *reinterpret_cast<int*>(sig + offset);
}

bool Utils::isLost(const HRESULT res) {
    return res == DXGI_ERROR_DEVICE_REMOVED || res == DXGI_ERROR_DEVICE_HUNG || res == DXGI_ERROR_DEVICE_RESET || res == DXGI_ERROR_DRIVER_INTERNAL_ERROR ||
        res == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;
}

bool Utils::isUsingDarkTheme() {
    const auto subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    const auto valueName = L"AppsUseLightTheme";

    DWORD data = 0;
    DWORD dataSize = sizeof(data);
    DWORD type = 0;
    const auto result = RegGetValueW(HKEY_CURRENT_USER, subKey, valueName, RRF_RT_REG_DWORD, &type, &data, &dataSize);

    if (result != ERROR_SUCCESS)
        return false;

    return data == 0;
}
