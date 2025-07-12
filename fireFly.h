#pragma once

#include <array>

class FireFly {
public:
	float x;
	float y;
	float speedx;
	float speedy;
	float radius;
	std::array<float, 4> color;

	FireFly(const FireFly&) = delete;
	FireFly();
	FireFly(float x, float y, float speed, float angle, float radius, std::array<float, 4> color);
	void move(float& dt) noexcept;
	void render(const int nSegments) const;
};
