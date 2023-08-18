#ifndef SHAPES_H
#define SHAPES_H

#include <glm/glm.hpp>
#include <vector>
#include <array> 
#include <SDL.h> 
#include "draw.h"

struct Fragment {
    glm::ivec2 position; // X and Y coordinates of the pixel (in screen space)

    // You can add other interpolated attributes here, such as color, texture coordinates, normals, etc.
    Color color; // Example: color of the pixel
    float depth;     // Depth value (Z) of the pixel in view/projection space

    // Default constructor
    Fragment() : position(0, 0), color(0,0,0,255), depth(0.0f) {}

    // Constructor to initialize the fragment's position and attributes
    Fragment(int x, int y, Color color, float fragmentDepth)
        : position(x, y), color(0,0,0,255),depth(fragmentDepth) {}
};



struct Uniforms {
  glm::mat4 modelMatrix;
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix;
  glm::mat4 viewPortMatrix;
};

Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms, Fragment& outFragment);
std::vector<std::vector<Vertex>> primitiveAssembly(const std::vector<Vertex>& transformedVertices);
Fragment fragmentShader(Fragment fragment);
// Declaración de la función rasterize



#endif // SHAPES_H
