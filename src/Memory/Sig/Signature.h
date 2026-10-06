#pragma once

class Signature {
public:
	template <size_t N>
	explicit Signature(hat::fixed_signature<N> signature) : signature(signature.begin(), signature.end()) {}

	void find();
	[[nodiscard]] bool isScanned() const;

	template <hat::fixed_string str>
	static Signature create() {
		return Signature(hat::compile_signature<str>());
	}

	uintptr_t addr = 0;
	hat::signature signature{};
	bool scanned = false;
};
