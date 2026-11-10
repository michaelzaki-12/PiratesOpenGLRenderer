#pragma once
#include <glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Texture.h"
#include "VAO.h"
#include "VBO.h"
#include "IBO.h"
#include "Shader.h"

#include <string>
#include <vector>

class Mesh {
public:
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, glm::mat4 meshTransformation);
    void Draw(Shader& shader, glm::vec3 scale);
    unsigned int VAO, VBO, IBO;
    glm::mat4 MeshTransformation;
private:
    void setupMesh();
};