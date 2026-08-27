#pragma once

template <typename T0>
class OwnerPtrT : public T0::OwnerStorage {
public:
    using OwnerStackRef = T0::OwnerStackRef;
    using Base = T0::OwnerStorage;
    using Base::Base;
};
