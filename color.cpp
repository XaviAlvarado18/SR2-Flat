#include "color.h"

// Define the friend function operator*(float, Color const&)
inline Color operator*(float factor, const Color& color) {
    return Color(
        std::clamp(static_cast<Uint8>(color.r * factor), Uint8(0), Uint8(255)),
        std::clamp(static_cast<Uint8>(color.g * factor), Uint8(0), Uint8(255)),
        std::clamp(static_cast<Uint8>(color.b * factor), Uint8(0), Uint8(255)),
        std::clamp(static_cast<Uint8>(color.a * factor), Uint8(0), Uint8(255))
    );
}