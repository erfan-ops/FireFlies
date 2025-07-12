#pragma once

#include <string>
#include <vector>
#include <array>

// Define a struct to represent your settings
struct Settings {
	float targetFPS;
	std::vector<std::array<float, 4>> backGroundColors;

	struct FireFlies {
		float maxRadius;
		int nSegments;
		std::vector<std::array<float, 4>> colors;
		float minAlpha;
		float maxAlpha;
		float brightenSpeed;
		float darkenSpeed;
		int count;
		float minSpeed;
		float maxSpeed;
	} fireFlies;

	float mouseRadius;
	float gravity;
	float minDistance;
};

// Function to load settings from a JSON file
Settings loadSettings(const std::string& filename);
