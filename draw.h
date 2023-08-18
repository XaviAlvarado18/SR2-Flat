#ifndef DRAW_H
#define DRAW_H

#include <vector>
#include "SDL2/SDL.h"
#include <glm/glm.hpp>
#include "color.h"

const int FRAMEBUFFER_WIDTH = 800; // Ancho del framebuffer
const int FRAMEBUFFER_HEIGHT = 600; 


class Vertex {
public:
    glm::vec3 position;
    Color color;

    Vertex(const glm::vec3& pos, Color color) : position(pos), color(255, 0, 0) {} // Constructor for 3D position
    Vertex(float x, float y, float z) : position(x, y, z) {}
    Vertex(const glm::vec2& pos) : position(pos.x, pos.y, 0.0f) {} // Constructor for screen-space position


    float getX() const { return position.x; }
    float getY() const { return position.y; }
};


#endif
