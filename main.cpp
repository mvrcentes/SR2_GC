#include <SDL.h>
#include <vector>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <filesystem>
#include "fragment.h"
#include "uniform.h"
#include <array>
#include <fstream>
#include <sstream>

#include "print.h"
#include "color.h"
#include "shaders.h"
#include "triangle.h"

const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 600;

std::array<std::array<float, WINDOW_WIDTH>, WINDOW_HEIGHT> zbuffer;

SDL_Renderer* renderer;

Uniform uniform;

struct Face {
    std::vector<std::array<int, 3>> vertexIndices;
};

std::string getCurrentPath() {
    return std::filesystem::current_path().string();
}

std::string getParentDirectory(const std::string& path) {
    std::filesystem::path filePath(path);
    return filePath.parent_path().string();
}

std::vector<glm::vec3> vertices;
std::vector<Face> faces;

// Declare a global clearColor of type Color
Color clearColor = {0, 0, 0}; // Initially set to black

// Declare a global currentColor of type Color
Color currentColor = {255, 255, 255}; // Initially set to white

// Function to clear the framebuffer with the clearColor
void clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    for (auto &row : zbuffer) {
        std::fill(row.begin(), row.end(), 99999.0f);
    }
}

// Function to set a specific pixel in the framebuffer to the currentColor
void point(Fragment f) {
    if (f.position.y < WINDOW_HEIGHT && f.position.x < WINDOW_WIDTH && f.position.y > 0 && f.position.x > 0 && f.position.z < zbuffer[f.position.y][f.position.x]) {
        SDL_SetRenderDrawColor(renderer, f.color.r, f.color.g, f.color.b, f.color.a);
        SDL_RenderDrawPoint(renderer, f.position.x, f.position.y);
        zbuffer[f.position.y][f.position.x] = f.position.z;
    }
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


void render(std::vector<glm::vec3> VBO) {
    // 1. Vertex Shader
    // vertex -> trasnformedVertices

    std::vector<Vertex> transformedVertices;

    for (int i = 0; i < VBO.size(); i++) {
        glm::vec3 v = VBO[i];

        Vertex vertex = {v, Color(255, 255, 255)};
        Vertex transformedVertex = vertexShader(vertex, uniform);
        transformedVertices.push_back(transformedVertex);
    }

    std::vector<std::vector<Vertex>> triangles = primitiveAssembly(transformedVertices);


    // 3. Rasterize
    // triangles -> Fragments
    std::vector<Fragment> fragments;
    for (const std::vector<Vertex>& triangleVertices : triangles) {
        std::vector<Fragment> rasterizedTriangle = triangle(
                triangleVertices[0],
                triangleVertices[1],
                triangleVertices[2]
        );

        fragments.insert(
                fragments.end(),
                rasterizedTriangle.begin(),
                rasterizedTriangle.end()
        );
    }

    // 4. Fragment Shader
    // Fragments -> colors

    for (Fragment fragment : fragments) {
        point(fragmentShader(fragment));
    }
}


float a = 3.14f / 3.0f;
float b = 0.5f / 3.0f;
glm::mat4 createModelMatrix() {
    b += 0.2f;
    glm::mat4 transtation = glm::translate(glm::mat4(1), glm::vec3(-0.05f, -0.09f, 0));
    glm::mat4 rotationy = glm::rotate(glm::mat4(1), glm::radians(a++), glm::vec3(0, 4, 0));
    glm::mat4 rotationx = glm::rotate(glm::mat4(1), glm::radians(b), glm::vec3(1, 0, 0));
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(0.15f, 0.15f, 0.15f));
    return transtation * scale * rotationx * rotationy ;
}

glm::mat4 createViewMatrix() {
    return glm::lookAt(
            // donde esta
            glm::vec3(10, 0, -1),
            // hacia a donde va 
            glm::vec3(10, 0, 0),
            // arriba
            glm::vec3(0, 1, 0)
    );
}

glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 45.0f;
    float aspectRatio = WINDOW_WIDTH / WINDOW_HEIGHT;
    float nearClip = 0.1f;
    float farClip = 100.0f;

    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);

    // Scale
    viewport = glm::scale(viewport, glm::vec3(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f, 0.5f));

    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}

// Función para leer el archivo .obj y cargar los vértices y caras
bool loadOBJ(const std::string& path, std::vector<glm::vec3>& out_vertices, std::vector<Face>& out_faces) {
    out_vertices.clear();
    out_faces.clear();

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            out_vertices.push_back(vertex);
        } else if (type == "f") {
            std::string lineHeader;
            Face face;
            while (iss >> lineHeader)
            {
                std::istringstream tokenstream(lineHeader);
                std::string token;
                std::array<int, 3> vertexIndices;

                // Read all three values separated by '/'
                for (int i = 0; i < 3; ++i) {
                    std::getline(tokenstream, token, '/');
                    vertexIndices[i] = std::stoi(token) - 1;
                }

                face.vertexIndices.push_back(vertexIndices);
            }
            out_faces.push_back(face);
        }
    }

    file.close();
    return true;
}

std::vector<glm::vec3> setupVertexArray(const std::vector<glm::vec3>& vertices, const std::vector<Face>& faces) {
    std::vector<glm::vec3> vertexArray;

    // For each face
    for (const auto& face : faces) {
        // For each vertex in the face
        for (const auto& vertexIndices : face.vertexIndices) {
            // Get the vertex position from the input array using the indices from the face
            glm::vec3 vertexPosition = vertices[vertexIndices[0]];

            // Add the vertex position to the vertex array
            vertexArray.push_back(vertexPosition);
        }
    }
    return vertexArray;
}

void writeBMP(const std::string& filename) {
    int width = WINDOW_WIDTH;
    int height = WINDOW_HEIGHT;

    float zMin = std::numeric_limits<float>::max();
    float zMax = std::numeric_limits<float>::lowest();

    for (const auto& row : zbuffer) {
        for (const auto& val : row) {
            if (val != 99999.0f) { // Ignora los valores que no han sido actualizados
                zMin = std::min(zMin, val);
                zMax = std::max(zMax, val);
            }
        }
    }

// Verifica que zMin y zMax sean diferentes
    if (zMin == zMax) {
        std::cerr << "zMin y zMax son iguales. Esto producirá una imagen en blanco o negro.\n";
        return;
    }

    std::cout << "zMin: " << zMin << ", zMax: " << zMax << "\n";


    // Abre el archivo en modo binario
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "No se pudo abrir el archivo para escribir: " << filename << "\n";
        return;
    }

    // Escribe la cabecera del archivo BMP
    uint32_t fileSize = 54 + 3 * width * height;
    uint32_t dataOffset = 54;
    uint32_t imageSize = 3 * width * height;
    uint32_t biPlanes = 1;
    uint32_t biBitCount = 24;

    uint8_t header[54] = {'B', 'M',
                          static_cast<uint8_t>(fileSize & 0xFF), static_cast<uint8_t>((fileSize >> 8) & 0xFF), static_cast<uint8_t>((fileSize >> 16) & 0xFF), static_cast<uint8_t>((fileSize >> 24) & 0xFF),
                          0, 0, 0, 0,
                          static_cast<uint8_t>(dataOffset & 0xFF), static_cast<uint8_t>((dataOffset >> 8) & 0xFF), static_cast<uint8_t>((dataOffset >> 16) & 0xFF), static_cast<uint8_t>((dataOffset >> 24) & 0xFF),
                          40, 0, 0, 0,
                          static_cast<uint8_t>(width & 0xFF), static_cast<uint8_t>((width >> 8) & 0xFF), static_cast<uint8_t>((width >> 16) & 0xFF), static_cast<uint8_t>((width >> 24) & 0xFF),
                          static_cast<uint8_t>(height & 0xFF), static_cast<uint8_t>((height >> 8) & 0xFF), static_cast<uint8_t>((height >> 16) & 0xFF), static_cast<uint8_t>((height >> 24) & 0xFF),
                          static_cast<uint8_t>(biPlanes & 0xFF), static_cast<uint8_t>((biPlanes >> 8) & 0xFF),
                          static_cast<uint8_t>(biBitCount & 0xFF), static_cast<uint8_t>((biBitCount >> 8) & 0xFF)};

    file.write(reinterpret_cast<char*>(header), sizeof(header));

    // Escribe los datos de los píxeles
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float normalized = (zbuffer[y][x] - zMin) / (zMax - zMin);
            uint8_t color = static_cast<uint8_t>(normalized * 255);
            file.write(reinterpret_cast<char*>(&color), 1);
            file.write(reinterpret_cast<char*>(&color), 1);
            file.write(reinterpret_cast<char*>(&color), 1);
        }
    }

    file.close();
    std::cout << "Archivo BMP guardado: " << filename<<"\n";
}

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window* window = SDL_CreateWindow("life", 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    std::string filePath = "/Users/mvrcentes/Library/CloudStorage/OneDrive-UVG/Documentos/Semestre_6/Graficas_por_computadoras/SR2_GC/x.obj";
    // std::string filePath = "/Users/mvrcentes/Downloads/quesOvni.obj";
    loadOBJ(filePath, vertices, faces);

    std::vector<glm::vec3> vertexArray = setupVertexArray(vertices, faces);

    renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED
    );

    bool running = true;
    SDL_Event event;

    std::vector<glm::vec3> vertexBufferObject = {
            {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
            {-0.87f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f},
            {0.87f,  -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f},

            {0.0f, 1.0f,    -1.0f}, {1.0f, 1.0f, 0.0f},
            {-0.87f, -0.5f, -1.0f}, {0.0f, 1.0f, 1.0f},
            {0.87f,  -0.5f, -1.0f}, {1.0f, 0.0f, 1.0f}
    };

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event. type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        break;
                    case SDLK_DOWN:
                        break;
                    case SDLK_LEFT:
                        break;
                    case SDLK_RIGHT:
                        break;
                    case SDLK_s:
                        break;
                    case SDLK_a:
                        break;
                }
            }
        }

        uniform.model = createModelMatrix();
        uniform.view = createViewMatrix();
        uniform.projection = createProjectionMatrix();
        uniform.viewport = createViewportMatrix();

        clear();

        // Call our render function
        render(vertexArray);
        // point(10, 10, Color{255, 255, 255});

        // Present the frame buffer to the screen
        SDL_RenderPresent(renderer);

        // Delay to limit the frame rate
        SDL_Delay(1000 / 60);
        writeBMP("zbuffer2.bmp");
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}