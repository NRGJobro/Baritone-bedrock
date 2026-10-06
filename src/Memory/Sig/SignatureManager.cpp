#include "SignatureManager.h"

SignatureManager sigMgr;

void SignatureManager::scanAll() {
    std::vector<std::future<void>> futures;

    futures.reserve(sigs.size());
    for (const auto& sig : std::views::values(sigs)) {
        if (!sig->isScanned()) {
            futures.push_back(std::async(std::launch::async, [sig] {
                sig->find();
            }));
        }
    }

    for (auto &future : futures)
        future.get();
}
