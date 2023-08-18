#include "SDL2/SDL.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring> // Agregamos esta línea para usar memcpy
#include <windows.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "draw.h"
#include "shapes.h"
#include <fstream>
#include <sstream>
#include <string>


const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 1200;
const int RENDER_SCALE = 45;
const int RENDER_WIDTH = WINDOW_WIDTH / RENDER_SCALE;
const int RENDER_HEIGHT = WINDOW_HEIGHT / RENDER_SCALE;

std::array<std::array<float, WINDOW_WIDTH>, WINDOW_HEIGHT> zbuffer;


SDL_Window* window;
SDL_Renderer* renderer;

Color currentColor = {255, 255, 255}; 

struct Face {
    std::vector<std::array<int, 3>> vertexIndices;
};

glm::vec3 barycentricCoordinates(const glm::vec3& P, const glm::vec3& A, const glm::vec3& B, const glm::vec3& C) {
    float w = ((B.y - C.y)*(P.x - C.x) + (C.x - B.x)*(P.y - C.y)) /
              ((B.y - C.y)*(A.x - C.x) + (C.x - B.x)*(A.y - C.y));

    float v = ((C.y - A.y)*(P.x - C.x) + (A.x - C.x)*(P.y - C.y)) /
              ((B.y - C.y)*(A.x - C.x) + (C.x - B.x)*(A.y - C.y));

    float u = 1.0f - w - v;

    return glm::vec3(w, v, u);
}

std::vector<Fragment> triangle(const Vertex& a, const Vertex& b, const Vertex& c) {
    std::vector<Fragment> fragments;

    // Calculate the bounding box of the triangle
    int minX = std::min(std::min(a.position.x, b.position.x), c.position.x);
    int maxX = std::max(std::max(a.position.x, b.position.x), c.position.x);
    int minY = std::min(std::min(a.position.y, b.position.y), c.position.y);
    int maxY = std::max(std::max(a.position.y, b.position.y), c.position.y);

    // Iterate over each point in the bounding box
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            glm::vec3 point(x, y, 0.0f);
            
            // Calculate barycentric coordinates for the point
            glm::vec3 barycentric = barycentricCoordinates(point, a.position, b.position, c.position);
            
            // If the point's barycentric coordinates are all between 0 and 1, it lies within the triangle
            if (barycentric.x >= 0.0f && barycentric.y >= 0.0f && barycentric.z >= 0.0f) {

                float xComponent = barycentric.x; // Obtiene el valor x (1.0f)
                float yComponent = barycentric.y; // Obtiene el valor y (2.0f)
                float zComponent = barycentric.z;
                // Interpolate attributes using barycentric coordinates
                Color interpolatedColor = xComponent * a.color + yComponent * b.color + zComponent * c.color;
                
                // Create a fragment and add it to the fragment list
                Fragment fragment(x, y, interpolatedColor, 0.0f); // Depth is set to 0 for now
                fragments.push_back(fragment);
            }
        }
    }
    
    return fragments;
}



// Function to set a specific pixel in the framebuffer to the currentColor
void point(Fragment f) {
    if (f.depth < zbuffer[f.position.y][f.position.x]) {
        SDL_SetRenderDrawColor(renderer, f.color.r, f.color.g, f.color.b, 255);
        SDL_RenderDrawPoint(renderer, f.position.x, f.position.y);
        zbuffer[f.position.y][f.position.x] = f.depth;
    }
}


bool isInsideTriangle(float x, float y, const Vertex& A, const Vertex& B, const Vertex& C) {
    float denominator = ((B.getY() - C.getY()) * (A.getX() - C.getX()) + (C.getX() - B.getX()) * (A.getY() - C.getY()));
    float a = ((B.getY() - C.getY()) * (x - C.getX()) + (C.getX() - B.getX()) * (y - C.getY())) / denominator;
    float b = ((C.getY() - A.getY()) * (x - C.getX()) + (A.getX() - C.getX()) * (y - C.getY())) / denominator;
    float c = 1.0f - a - b;

    return a >= 0 && a <= 1 && b >= 0 && b <= 1 && c >= 0 && c <= 1;
}



bool loadOBJ(const std::string& path, std::vector<glm::vec3>& out_vertices, std::vector<Face>& out_faces) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            out_vertices.emplace_back(x, y, z);
        } else if (type == "f") {
            Face face;
            char delimiter;
            std::array<int, 3> vertexIndices;

           for (int i = 2; i >= 0; --i) {
            std::string vertexIndexStr;
            iss >> vertexIndexStr;
            std::istringstream vertexIndexStream(vertexIndexStr);
            vertexIndexStream >> vertexIndices[i];
            vertexIndices[i]--; // Subtract 1 to match C++ indexing
            vertexIndexStream >> delimiter; // Skip texture and normal indices
        }


            face.vertexIndices.emplace_back(vertexIndices);
            out_faces.push_back(face);
        }
    }

    file.close();
    return true;
}

std::vector<Fragment> rasterize(const std::vector<std::vector<Vertex>>& assembledVertices) {
    std::vector<Fragment> fragments;

    for (const std::vector<Vertex>& triangleVertices : assembledVertices) {
        std::vector<Fragment> triangleFragments = triangle(triangleVertices[0], triangleVertices[1], triangleVertices[2]);
        fragments.insert(fragments.end(), triangleFragments.begin(), triangleFragments.end());
    }

    return fragments;
}


glm::mat4 createModelMatrix(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    // Aplicar transformaciones en el orden: escala, rotación, traslación
    modelMatrix = glm::translate(modelMatrix, translation);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = glm::scale(modelMatrix, scale);

    return modelMatrix;
}

glm::mat4 createProjectionMatrix(float fovInDegrees, float aspectRatio, float nearClip, float farClip) {
    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

glm::mat4 createViewMatrix() {
    return glm::lookAt(
        // donde esta
        glm::vec3(0, 0, -5),
        // hacia adonde mira
        glm::vec3(0, 0, 0),
        // arriba
        glm::vec3(0, 1, 0)
    );
}


glm::mat4 createViewportMatrix(float screenWidth, float screenHeight) {
    glm::mat4 viewport = glm::mat4(1.0f);

    // Scale
    viewport = glm::scale(viewport, glm::vec3(screenWidth / 2.0f, screenHeight / 2.0f, 0.5f));

    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}


void render(const std::vector<glm::vec3>& vertices, const Uniforms& uniforms) {
    // Step 1: Vertex Shader
    std::vector<Vertex> transformedVertices;
    Fragment fragments_st;


    for (int i = 0; i < vertices.size(); i+=2) {
        glm::vec3 v = vertices[i];
        glm::vec3 c = vertices[i+1];

        Vertex vertex = {v}; // Initialize with position only
        vertex.color = Color(c.x, c.y, c.z);
        Vertex transformedVertex = vertexShader(vertex, uniforms,fragments_st);
        transformedVertices.push_back(transformedVertex);
    }


    std::vector<std::vector<Vertex>> assembledVertices = primitiveAssembly(transformedVertices);


    // Step 2: Rasterization
    std::vector<Fragment> rasterizedFragments = rasterize(assembledVertices);

    for (Fragment fragment : rasterizedFragments) {
        point(fragmentShader(fragment));
    }
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Blender's Model", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    //UNIFORMS Y ARREGLOS
    std::vector<glm::vec3> vertices = {
         {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        {-0.87f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f},
        {0.87f,  -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f},

        {0.0f, 1.0f,    -1.0f}, {1.0f, 1.0f, 0.0f},
        {-0.87f, -0.5f, -1.0f}, {0.0f, 1.0f, 1.0f},
        {0.87f,  -0.5f, -1.0f}, {1.0f, 0.0f, 1.0f}
    };

    // Create a Uniforms instance with the necessary matrices
    Uniforms shaderUniforms;

    shaderUniforms.modelMatrix = createModelMatrix(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    shaderUniforms.viewMatrix = createViewMatrix();
    shaderUniforms.viewPortMatrix = createViewportMatrix(WINDOW_WIDTH , WINDOW_HEIGHT);
    shaderUniforms.projectionMatrix = createProjectionMatrix(45.0,1400/1200,0.1f, 100.0f);

    //PASARLO A RENDER LUEGO EL SHADERUNIFORMS


    SDL_Event event;
    bool quit = false;

    std::vector<glm::vec3> verticesO;
    std::vector<Face> faces;

    if (!loadOBJ("../navecita.obj", verticesO, faces)) {
        std::cerr << "Error loading OBJ file." << std::endl;
        return 1;
    }

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render(vertices,shaderUniforms);

        //drawModel(verticesO, faces);

        SDL_RenderPresent(renderer);

    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}