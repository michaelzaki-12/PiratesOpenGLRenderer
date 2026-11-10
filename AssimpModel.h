#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "Shader.h"
#include "Mesh.h"
#include <assimp/Importer.hpp>
#include "../PCH.hpp"
#include <glm/glm.hpp>
class AssimpModel
{
public:
    AssimpModel(std::string path);
    void Draw(Shader& shader);
    static inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }
    // model data
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;
    glm::vec3 scale = glm::vec3(0.0f);
    bool changescale = false;
private:
    glm::mat4 globaltransformation = glm::mat4(1.0f);
    unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma);
    void loadModel(std::string path);
    void processNode(aiNode* node, const aiScene* scene, glm::mat4 parent_transformation);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene, glm::mat4& transformation);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
        std::string typeName);

};
