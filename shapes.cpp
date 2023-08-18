#include "shapes.h"
#include <glm/glm.hpp>

Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms, Fragment& outFragment) {
    
    // Extract the position from the vertex
    glm::vec3 position = vertex.position;

    // Apply transformations (model, view, projection) to the vertex
    glm::vec4 transformedVertex = uniforms.projectionMatrix * uniforms.viewMatrix * uniforms.modelMatrix * glm::vec4(position, 1.0f);
    
    // Homogeneous divide
    glm::vec3 ndcVertex = glm::vec3(transformedVertex) / transformedVertex.w;
    

    // Apply the viewport transform
    glm::vec4 screenVertex = uniforms.viewPortMatrix * glm::vec4(ndcVertex, 1.0f);

    return Vertex{
        glm::vec3(screenVertex ),
        vertex.color
    };
}

std::vector<std::vector<Vertex>> primitiveAssembly(
    const std::vector<Vertex>& transformedVertices
) {
    std::vector<std::vector<Vertex>> groupedVertices;

    for (int i = 0; i < transformedVertices.size(); i += 3) {
        std::vector<Vertex> vertexGroup;
        vertexGroup.push_back(transformedVertices[i]);
        vertexGroup.push_back(transformedVertices[i+1]);
        vertexGroup.push_back(transformedVertices[i+2]);
        
        groupedVertices.push_back(vertexGroup);
    }

    return groupedVertices;
}


Fragment fragmentShader(Fragment fragment) {
  return fragment;
};

