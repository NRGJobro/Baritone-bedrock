#include "Signature.h"

#include "../../Utils/Utils.h"

void Signature::find() {
    this->addr = Utils::findSig(this->signature);
    this->scanned = true;
}

bool Signature::isScanned() const {
    return this->scanned;
}

bool Signature::isValid() const {
    return this->addr != 0;
}
