#pragma once

class HashedString {
public:
	uint64_t hash;
	std::string str;
	HashedString* lastMatch;

	constexpr static uint64_t computeHash(std::string_view str) {
		uint64_t hash = 0xCBF29CE484222325ULL;
		for (char s : str) {
			hash = s ^ (0x100000001B3ULL * hash);
		}
		return hash;
	}

	constexpr HashedString(std::nullptr_t = nullptr) noexcept : hash(0), lastMatch(nullptr) {}

	constexpr HashedString(uint64_t h, char const* str) noexcept : hash(h), str(str), lastMatch(nullptr) {}

	constexpr HashedString(char const* str) noexcept : hash(computeHash(str)), str(str), lastMatch(nullptr) {}

	constexpr HashedString(std::string const& str) noexcept
		: hash(computeHash(str)),
		  str(str),
		  lastMatch(nullptr) {}

	constexpr HashedString(HashedString const& other) noexcept : hash(other.hash), str(other.str), lastMatch(nullptr) {}

	constexpr HashedString(HashedString&& other) noexcept
		: hash(other.hash),
		  str(std::move(other.str)),
		  lastMatch(other.lastMatch) {
		other.hash = 0;
		other.lastMatch = nullptr;
	}

	constexpr HashedString& operator=(HashedString const& other) noexcept {
		if (this == &other) {
			return *this;
		}
		hash = other.hash;
		str = other.str;
		lastMatch = nullptr;
		return *this;
	}

	constexpr HashedString& operator=(HashedString&& other) noexcept {
		hash = other.hash;
		str = std::move(other.str);
		lastMatch = other.lastMatch;
		other.hash = 0;
		other.lastMatch = nullptr;
		return *this;
	}

	constexpr char const* c_str() const noexcept { return str.c_str(); }

	constexpr std::string const& getString() const noexcept { return str; }

	constexpr uint64_t getHash() const noexcept { return hash; }

	constexpr bool isEmpty() const noexcept { return str.empty(); }

	constexpr void clear() noexcept {
		hash = 0;
		str.clear();
		lastMatch = nullptr;
	}

	template <typename StringType>
	constexpr bool operator==(StringType const& rhs) const noexcept {
		return str == rhs;
	}

	constexpr bool operator==(HashedString const& other) const noexcept { return hash == other.hash; }

	template <typename StringType>
	constexpr bool operator!=(StringType const& rhs) const noexcept {
		return str != rhs;
	}

	constexpr bool operator!=(HashedString const& other) const noexcept { return hash != other.hash; }

	template <typename StringType>
	constexpr std::strong_ordering operator<=>(StringType const& other) const noexcept {
		return str <=> other.str;
	}

	constexpr std::strong_ordering operator<=>(HashedString const& other) const noexcept { return str <=> other.str; }

	constexpr explicit operator std::string() const { return str; }

	constexpr explicit operator std::string_view() const { return std::string_view(str); }
};

namespace std {
    template <>
    struct hash<HashedString> {
        size_t operator()(HashedString const& str) const noexcept { return str.getHash(); }
    };
}
