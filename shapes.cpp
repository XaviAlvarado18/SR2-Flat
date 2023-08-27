#include "shapes.h"
#include <glm/glm.hpp>

Vertex vertexShader(const Vertex& vertex, const Uniforms& uniforms) {
    glm::vec4 transformedVertex = uniforms.viewport * uniforms.projection * uniforms.view * uniforms.model * glm::vec4(vertex.position, 1.0f);
    glm::vec3 vertexRedux;
    vertexRedux.x = transformedVertex.x / transformedVertex.w;
    vertexRedux.y = transformedVertex.y / transformedVertex.w;
    vertexRedux.z = transformedVertex.z / transformedVertex.w;
    Color fragmentColor(253, 221, 202, 255); 
    glm::vec3 normal = glm::normalize(glm::mat3(uniforms.model) * vertex.normal);
    // Create a fragment and assign its attributes
    Fragment fragment;
    fragment.position = glm::ivec2(transformedVertex.x, transformedVertex.y);
    fragment.color = fragmentColor;
    return Vertex {vertexRedux, normal};
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


Color fragmentShader(const Fragment& fragment) {
    // Example: Assign a constant color to each fragment
    // You can modify this function to implement more complex shading
    // based on the fragment's attributes (e.g., depth, interpolated normals, texture coordinates, etc.)

    return fragment.color;
}
