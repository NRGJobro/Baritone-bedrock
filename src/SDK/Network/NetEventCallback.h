#pragma once

#include "../Bedrock/EnableNonOwnerReferences.h"

class NetEventCallback : public Bedrock::EnableNonOwnerReferences {
    virtual void Destructor();
};
