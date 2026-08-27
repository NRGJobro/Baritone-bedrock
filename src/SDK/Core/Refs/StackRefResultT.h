#pragma once

template <typename T>
class StackRefResultT : public T::StackResultStorage {
public:
    using StackRef = T::StackRef;
};
