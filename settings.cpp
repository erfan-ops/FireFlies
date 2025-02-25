#include "settings.h"
#include <fstream>
#include <nlohmann/json.hpp>

// Function to load settings from a JSON file
Settings loadSettings(const std::string& filename) {
	std::ifstream file(filename);
	nlohmann::json j;
	file >> j;

	Settings settings;

	settings.targetFPS = j["fps"];
	settings.backGroundColors = j["background-colors"].get<std::vector<std::array<float, 4>>>();

	settings.fireFlies.maxRadius = j["fireflies"]["max-radius"];
	settings.fireFlies.nSegments = j["fireflies"]["segments"];
	settings.fireFlies.colors = j["fireflies"]["colors"].get<std::vector<std::array<float, 4>>>();
	settings.fireFlies.minAlpha = j["fireflies"]["min-alpha"];
	settings.fireFlies.maxAlpha = j["fireflies"]["max-alpha"];
	settings.fireFlies.brightenSpeed = j["fireflies"]["increase-brightness-speed"];
	settings.fireFlies.darkenSpeed = j["fireflies"]["decrease-brightness-speed"];
	settings.fireFlies.count = j["fireflies"]["count"];
	settings.fireFlies.maxSpeed = j["fireflies"]["max-speed"];

	settings.mouseRadius = j["mouse-radius"];
	settings.gravity = j["mouse-gravity"];
	settings.minDistance = j["min-distance"];

	return settings;
}
