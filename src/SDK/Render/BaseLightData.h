#pragma once

#include "../Client/MCE/Color.h"

struct BaseLightData {
	struct DarknessLevels {
		float currentLevel;
		float prevLevel;
	};

	mce::Color sunriseColor;
	float gamma;
	float skyDarken;
	int dimensionType;
	float darkenWorldAmount;
	float previousDarkenWorldAmount;
	bool nightvisionActive;
	float nightvisionScale;
	bool underwaterVision;
	float underwaterScale;
	DarknessLevels darknessLevels;
	int skyFlashTime;
};
