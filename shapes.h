#ifndef SHAPES_H
#define SHAPES_H

#include <glm/glm.hpp>
#include <vector>
#include <array> 
#include <SDL.h>
#include "draw.h"

struct Fragment {
    glm::ivec2 position; // X and Y coordinates of the pixel (in screen space)
    Color color;
    float z;
};


struct Uniforms {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewport;
};

Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms);
std::vector<std::vector<Vertex>> primitiveAssembly(const std::vector<Vertex>& transformedVertices);
Color fragmentShader(const Fragment& fragment);
// Declaración de la función rasterize



#endif // SHAPES_H
