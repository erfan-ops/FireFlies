#include "fireFly.h"
#include "rendering.h"


FireFly::FireFly(float x, float y, float speedx, float speedy, float radius, std::array<float, 4> color) :
    x(x),
    y(y),
    speedx(speedx),
    speedy(speedy),
    radius(radius),
    color(color)
{}
FireFly::FireFly() : x(0.0f), y(0.0f), speedx(0.0f), speedy(0.0f), radius(0.0f), color({ 0.0f, 0.0f , 0.0f , 0.0f }) {}

void FireFly::move(float& dt) noexcept {
    this->x += this->speedx * dt;
    this->y += this->speedy * dt;
}

void FireFly::render(const int nSegments) const {
    filledCircle(x, y, radius * color[3], color[0], color[1], color[2], color[3], nSegments);
}
