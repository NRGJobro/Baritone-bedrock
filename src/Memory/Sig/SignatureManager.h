#pragma once

#include "Signature.h"
#include "../../Utils/Logger.h"

constexpr size_t fnv1a_64_hash(const std::string_view str, const size_t hash = 14695981039346656037ull) {
    return str.size() == 0 ? hash : fnv1a_64_hash(str.substr(1), (hash ^ str[0]) * 1099511628211ull);
}

consteval size_t compile_time_hash(const std::string_view str) {
    return fnv1a_64_hash(str);
}

#define ADD_SIG(name, sig) sigMgr.addSignature<name>(Signature::create<sig>())
#define GET_SIG(name) sigMgr.getSig<name>()

class SignatureManager {
public:
    template<hat::fixed_string name>
    void addSignature(Signature&& signature) {
        auto failed = sigs.emplace(compile_time_hash(name.to_view()), std::make_shared<Signature>(std::move(signature)));
        assert(failed.second && "Failed to add signature");
    }

    template<hat::fixed_string name>
    [[nodiscard]] uintptr_t getSig() const {
        if (!sigs.contains(compile_time_hash(name.to_view()))) {
#ifndef NDEBUG
            logF("Couldn't find sig: {}", name.c_str());
#endif
            return 0;
        }

        const auto& sig = sigs.at(compile_time_hash(name.to_view()));

        if (!sig->isScanned())
            sig->find();

        return sig->addr;
    }

    void scanAll();

private:
    std::unordered_map<size_t, std::shared_ptr<Signature>> sigs;
};

extern SignatureManager sigMgr;
