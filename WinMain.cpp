#include "SDL2/SDL.h"
#include <ctime>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "color.h"
#include <vector>
#include "draw.h"
#include "shapes.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <sstream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

Color clearColor = {0, 0, 0, 255};
Color currentColor = {255, 255, 255, 255};
Color colorA(253, 221, 202, 255); 
Color colorB(0, 255, 255, 255); 
Color colorC(0, 24, 255, 255); 
glm::vec3 L = glm::vec3(0, 0, 200.0f); 

Uniforms uniforms;
SDL_Renderer* renderer;

std::array<double, WINDOW_WIDTH * WINDOW_HEIGHT> zbuffer;

struct Face {
    std::array<int, 3> vertexIndices;
    std::array<int, 3> normalIndices;
};


Color interpolateColor(const glm::vec3& barycentricCoord, const Color& colorA, const Color& colorB, const Color& colorC) {
    float u = barycentricCoord.x;
    float v = barycentricCoord.y;
    float w = barycentricCoord.z;

    // Realiza una interpolación lineal para cada componente del color
    uint8_t r = static_cast<uint8_t>(u * colorA.r + v * colorB.r + w * colorC.r);
    uint8_t g = static_cast<uint8_t>(u * colorA.g + v * colorB.g + w * colorC.g);
    uint8_t b = static_cast<uint8_t>(u * colorA.b + v * colorB.b + w * colorC.b);
    uint8_t a = static_cast<uint8_t>(u * colorA.a + v * colorB.a + w * colorC.a);

    return Color(r, g, b, a);
}

bool isBarycentricCoordInsideTriangle(const glm::vec3& barycentricCoord) {
    return barycentricCoord.x >= 0 && barycentricCoord.y >= 0 && barycentricCoord.z >= 0 &&
           barycentricCoord.x <= 1 && barycentricCoord.y <= 1 && barycentricCoord.z <= 1 &&
           glm::abs(1 - (barycentricCoord.x + barycentricCoord.y + barycentricCoord.z)) < 0.00001f;
}

glm::vec3 calculateBarycentricCoord(const glm::vec2& A, const glm::vec2& B, const glm::vec2& C, const glm::vec2& P) {
    float denominator = (B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y);
    float u = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / denominator;
    float v = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / denominator;
    float w = 1 - u - v;
    return glm::vec3(u, v, w);
}

std::vector<Fragment> triangle(const Vertex& a, const Vertex& b, const Vertex& c) {
    std::vector<Fragment> fragments;

    int minX = static_cast<int>(std::min({a.position.x, b.position.x, c.position.x}));
    int minY = static_cast<int>(std::min({a.position.y, b.position.y, c.position.y}));
    int maxX = static_cast<int>(std::max({a.position.x, b.position.x, c.position.x}));
    int maxY = static_cast<int>(std::max({a.position.y, b.position.y, c.position.y}));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            glm::vec2 pixelPosition(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f); // Central point of the pixel
            glm::vec3 barycentricCoord = calculateBarycentricCoord(a.position, b.position, c.position, pixelPosition);

            if (isBarycentricCoordInsideTriangle(barycentricCoord)) {
                Color p {0, 0, 0};
                Color interpolatedColor = interpolateColor(barycentricCoord, p, p, p);
                float interpolatedZ = barycentricCoord.x * a.position.z + barycentricCoord.y * b.position.z + barycentricCoord.z * c.position.z;
                Fragment fragment;
                fragment.position = glm::ivec2(x, y);
                fragment.color = interpolatedColor;
                fragment.z = interpolatedZ;

                fragments.push_back(fragment);
            }
        }
    }

    return fragments;
}

void render(const std::vector<Vertex>& vertexArray,  const Uniforms& uniforms) {
    std::vector<Vertex> transformedVertexArray;
    for (const auto& vertex : vertexArray) {
        auto transformedVertex = vertexShader(vertex, uniforms);
        transformedVertexArray.push_back(transformedVertex);
    }

    std::fill(zbuffer.begin(), zbuffer.end(), std::numeric_limits<double>::max());


    for (size_t i = 0; i < transformedVertexArray.size(); i += 3) {
        const Vertex& a = transformedVertexArray[i];
        const Vertex& b = transformedVertexArray[i + 1];
        const Vertex& c = transformedVertexArray[i + 2];

        glm::vec3 A = a.position;
        glm::vec3 B = b.position;
        glm::vec3 C = c.position;

        int minX = static_cast<int>(std::min({A.x, B.x, C.x}));
        int minY = static_cast<int>(std::min({A.y, B.y, C.y}));
        int maxX = static_cast<int>(std::max({A.x, B.x, C.x}));
        int maxY = static_cast<int>(std::max({A.y, B.y, C.y}));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                glm::vec2 pixelPosition(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f); // Central point of the pixel
                glm::vec3 barycentricCoord = calculateBarycentricCoord(A, B, C, pixelPosition);

                if (isBarycentricCoordInsideTriangle(barycentricCoord)) {
                    Color g {200,0,0};
                    Color interpolatedColor = interpolateColor(barycentricCoord, g, g, g);

                    float depth = barycentricCoord.x * A.z + barycentricCoord.y * B.z + barycentricCoord.z * C.z;


                    glm::vec3 normal = a.normal * barycentricCoord.x + b.normal * barycentricCoord.y+ c.normal * barycentricCoord.z;

                    float fragmentIntensity = glm::dot(normal, glm::vec3 (0,0,1.0f));
                    Color finalColor = interpolatedColor * fragmentIntensity;
                    Color finalColor2(253, 221, 202, 255);

                    Fragment fragment;
                    fragment.position = glm::ivec2(x, y);
                    fragment.color = finalColor;
                    fragment.z = depth;  // Set the depth of the fragment

                    int index = y * WINDOW_WIDTH + x;
                    if (depth < zbuffer[index]) {
                        Color fragmentShaderf = fragmentShader(fragment);

                        

                        SDL_SetRenderDrawColor(renderer, fragmentShaderf.r, fragmentShaderf.g, fragmentShaderf.b, fragmentShaderf.a);
                        //SDL_SetRenderDrawColor(renderer, 253, 221, 202, 255);
                        SDL_RenderDrawPoint(renderer, x, y);

                        // Update the z-buffer value for this pixel
                        zbuffer[index] = depth;
                    }
                }
            }
        }
    }
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);

    // Scale
    viewport = glm::scale(viewport, glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0.5f));

    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}

glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 45.0f;
    float aspectRatio = WINDOW_WIDTH / WINDOW_HEIGHT;
    float nearClip = 0.1f;
    float farClip = 100.0f;

    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

float a = 3.14f / 3.0f;

glm::mat4 createModelMatrix() {
    glm::mat4 transtation = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(1.0f, 1.0f, 1.0f));
    glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians((a++)), glm::vec3(0, 1.0f, 0.0f));
    return transtation * scale * rotation;
}

std::vector<Vertex> setupVertexArray(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals, const std::vector<Face>& faces)
{
    std::vector<Vertex> vertexArray;

    float scale = 1.0f;

    for (const auto& face : faces)
    {
        glm::vec3 vertexPosition1 = vertices[face.vertexIndices[0]];
        glm::vec3 vertexPosition2 = vertices[face.vertexIndices[1]];
        glm::vec3 vertexPosition3 = vertices[face.vertexIndices[2]];

        glm::vec3 normalPosition1 = normals[face.normalIndices[0]];
        glm::vec3 normalPosition2 = normals[face.normalIndices[1]];
        glm::vec3 normalPosition3 = normals[face.normalIndices[2]];

        vertexArray.push_back(Vertex {vertexPosition1, normalPosition1});
        vertexArray.push_back(Vertex {vertexPosition2, normalPosition2});
        vertexArray.push_back(Vertex {vertexPosition3, normalPosition3});


    }

    return vertexArray;
}

bool loadOBJ(const char* path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec3>& out_normals, std::vector<Face>& out_faces)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cout << "Failed to open the file: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string lineHeader;
        iss >> lineHeader;

        if (lineHeader == "v")
        {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            out_vertices.push_back(vertex);
        }
        else if (lineHeader == "vn")
        {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            out_normals.push_back(normal);
        }
        else if (lineHeader == "f")
        {
            Face face;
            for (int i = 0; i < 3; ++i)
            {
                std::string faceData;
                iss >> faceData;

                std::replace(faceData.begin(), faceData.end(), '/', ' ');

                std::istringstream faceDataIss(faceData);
                int temp; // for discarding texture indices
                faceDataIss >> face.vertexIndices[i] >> temp >> face.normalIndices[i];

                face.vertexIndices[i]--;
                face.normalIndices[i]--;
            }
            out_faces.push_back(face);
        }
    }

    return true;
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    SDL_Init(SDL_INIT_EVERYTHING);

    glm::vec3 translation(0.0f, 0.0f, 0.0f); 
    glm::vec3 rotationAngles(0.0f, 0.0f, 0.0f); 
    glm::vec3 scale(1.0f, 1.0f, 1.0f);


    glm::vec3 cameraPosition(0.0f, 0.0f, 5.0f); 
    glm::vec3 targetPosition(0.0f, 0.0f, 0.0f);   
    glm::vec3 upVector(0.0f, 1.0f, 0.0f);

    uniforms.view = glm::lookAt(cameraPosition, targetPosition, upVector);

    srand(time(nullptr));

    SDL_Window* window = SDL_CreateWindow("Pixel Drawer", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    int renderWidth, renderHeight;
    SDL_GetRendererOutputSize(renderer, &renderWidth, &renderHeight);

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normal;
    std::vector<Face> faces;

    bool success = loadOBJ("../Wolf_One_obj.obj", vertices, normal, faces);
    if (!success) {
        return 1;
    }

    glm::mat4 additionalRotation = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    for (glm::vec3& vertex : vertices) {
        vertex = glm::vec3(additionalRotation * glm::vec4(vertex, 1.0f));
    }

    std::vector<Vertex> vertexArray = setupVertexArray(vertices, normal, faces);

    bool running = true;
    SDL_Event event;
    glm::mat4 rotationMatrix = glm::mat4(1.0f); 
    glm::mat4 traslateMatrix = glm::mat4(1.0f); 
    glm::mat4 scaleMatrix = glm::mat4(1.0f); 

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        uniforms.model = createModelMatrix();    
        uniforms.projection = createProjectionMatrix();
        uniforms.viewport = createViewportMatrix();


        SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        SDL_RenderClear(renderer);

        glm::vec4 transformedLight = glm::inverse(createModelMatrix()) * glm::vec4(L, 0.0f);
        glm::vec3 transformedLightDirection = glm::normalize(glm::vec3(transformedLight));

        render(vertexArray, uniforms);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}