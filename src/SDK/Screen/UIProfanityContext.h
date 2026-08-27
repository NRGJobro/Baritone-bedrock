#pragma once

#include "../Bedrock/NonOwnerPointer.h"

enum ProfanityFilterContext : int32_t {
	None = 0,
	UIFrontEnd = 1,
	UIInGame = 2,
	AllUI = 3,
	InGameChat = 4,
	InGameItems = 8,
	InGameName = 16,
	All = 31
};

class UIProfanityContext : public Bedrock::EnableNonOwnerReferences {
public:
	bool enabled;
    bool remoteFilterEnabled;
    bool playerProfanityFilterEnabled;
	ProfanityFilterContext filterMask;
	std::unordered_map<std::string, int32_t> profanityExactMap;
	std::unordered_set<std::string> profanityContainsSet;

	virtual ~UIProfanityContext();
	virtual std::string* _doMaskProfanity(std::string const& str);
	virtual bool _doFindProfanity(std::string const& str);
};