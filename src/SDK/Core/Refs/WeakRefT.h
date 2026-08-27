#pragma once

template <typename T>
class WeakRefT : public T::WeakStorage {};
