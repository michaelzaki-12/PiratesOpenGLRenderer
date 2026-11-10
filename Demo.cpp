#include "glad.h"
#include <GLFW/glfw3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#include "Deps/stb_write_image.h"
#include "Shader.h"
#include "ComputeShader.h"
#include "VAO.h"
#include "VBO.h"
#include "Texture.h"
#include "Mesh.h"
#include "AssimpModel.h"
#include "Demo.h"
#include <filesystem>
#include "Deps/Camera.h"
#define STBI_FAILURE_USERMSG
#include "../assimp/include/assimp/DefaultLogger.hpp"
#include "Deps/stb_image.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include <map>
#ifdef WIN32
#include "Windows.h"
#endif
#ifdef LINUX

#endif
//put those here until i figure out a better place when refactoring my code!
struct alignas(16) VolumeTileAABB{
    glm::vec4 minPoint;
    glm::vec4 maxPoint;
    uint count;
    uint lightIndices[100];
} frustrum;

void setmodeltranslationandscale(glm::mat4& model, glm::vec3 scale, glm::vec3 translation, glm::mat4 rotation);
void renderquad(unsigned int quadVAO,Texture texture, Texture NormalTexture, unsigned int depthCubeMap);

struct alignas(16) PointLight
{
    glm::vec4 position;
    glm::vec4 Color;
    float intensity;
    float radius;
};

void PrintMat4(const glm::mat4& mat) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            // Access elements using mat[column][row] due to GLM's column-major storage
            std::cout << mat[j][i] << "\t";
        }
        std::cout << std::endl;
    }
}
//------------------------------------------------------------------------
//Needs Modifications to suit directional light.
/*
struct dirlight
{
    glm::vec3 direction;
    glm::vec3 Color;
    float Constant;
    float Linear;
    float Quadratic;

};
*/

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfw window creation
    // --------------------
    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    //glfw window initionally width and height
    int Width = 800, Height = 600;
    GLFWwindow* window = glfwCreateWindow(Width, Height, "Pirates Engine", NULL, NULL);

    //loading our Engine Icon
    Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
    Assimp::LogStream* stderrStream = Assimp::LogStream::createDefaultStream(aiDefaultLogStream_STDERR);
    Assimp::DefaultLogger::get()->attachStream(stderrStream, Assimp::Logger::NORMAL | Assimp::Logger::DEBUGGING | Assimp::Logger::VERBOSE);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    camera.width = Width;
    camera.height = Height;
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    float zNear = 1.0f, zFar = 100.0f;
    glViewport(0, 0, 800, 600);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4   projection = glm::perspective(glm::radians(45.0f), (float)Width / (float)Height, zNear, zFar);
    glm::mat4 view = glm::mat4(1.0f);
    bool gamma = true;
    // build and compile our shader program
    // ------------------------------------
    Shader ourShader;
    ourShader.Init("ShaderSrc/default.vert", "ShaderSrc/default.frag");

    Shader Framebuffer;
    Framebuffer.Init("ShaderSrc/liningShader.vert", "ShaderSrc/liningShader.frag");

    Shader SimpleShader;
    SimpleShader.Init("ShaderSrc/SimpleShader.vert", "ShaderSrc/SimpleShader.frag");

    //Shader For Debugging, not important right now.
    //Shader DepthDebug;
    //DepthDebug.Init("ShaderSrc/DepthDebug.vert", "ShaderSrc/DepthDebug.frag");

    //DepthDebug.use();
    //DepthDebug.setInt("depthMap", 0);

    Shader LightShader;
    LightShader.Init("ShaderSrc/light.vert", "ShaderSrc/light.frag");

    ComputeShader Clusters("ShaderSrc/Clusters.comp");


    ComputeShader CullEngine("ShaderSrc/Cull.comp");

    Shader hdrskybox;
    hdrskybox.Init("ShaderSrc/EquirectangularToCubemap.vert", "ShaderSrc/EquirectangularToCubemap.frag");

    Shader HdrCubeMap;
    HdrCubeMap.Init("ShaderSrc/HdrCubeMap.vert", "ShaderSrc/HdrCubeMap.frag");

    Shader skybox;
    skybox.Init("ShaderSrc/skybox.vert", "ShaderSrc/skybox.frag");


    std::vector<std::string> faces = {
        "skybox/right.jpg",
        "skybox/left.jpg",
        "skybox/top.jpg",
        "skybox/bottom.jpg",
        "skybox/front.jpg",
        "skybox/back.jpg"
    };

    std::string hdrface = {
        "wildflower_field_2k.hdr"
    };

    AssimpModel Sponza("Sponza/Sponza.gltf");

    float skyboxVertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // Our light Cube vertex
    std::vector<Vertex> Cubevertices = {
        // COORDINATES                         //Normals                            // TexCoord
        Vertex{glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3( 0.0f,  0.0f, -1.0f),  glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3( 0.5f,  0.5f, -0.5f),  glm::vec3( 0.0f,  0.0f, -1.0f),  glm::vec2(1.0f, 1.0f)},
        Vertex{glm::vec3( 0.5f, -0.5f, -0.5f),  glm::vec3( 0.0f,  0.0f, -1.0f),  glm::vec2(1.0f, 0.0f)},
        Vertex{glm::vec3( 0.5f,  0.5f, -0.5f),  glm::vec3( 0.0f,  0.0f, -1.0f),  glm::vec2(1.0f, 1.0f)}, //good
        Vertex{glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3( 0.0f,  0.0f, -1.0f),  glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3( 0.0f,  0.0f, -1.0f),  glm::vec2(0.0f, 1.0f)},
        Vertex{glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3( 0.0f,  0.0f,  1.0f),  glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3( 0.5f, -0.5f,  0.5f),  glm::vec3( 0.0f,  0.0f,  1.0f),  glm::vec2(1.0f, 0.0f)},
        Vertex{glm::vec3( 0.5f,  0.5f,  0.5f),  glm::vec3( 0.0f,  0.0f,  1.0f),  glm::vec2(1.0f, 1.0f)}, // good
        Vertex{glm::vec3( 0.5f,  0.5f,  0.5f),  glm::vec3( 0.0f,  0.0f,  1.0f),  glm::vec2(1.0f, 1.0f)},
        Vertex{glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3( 0.0f,  0.0f,  1.0f),  glm::vec2(0.0f, 1.0f)},
        Vertex{glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3( 0.0f,  0.0f,  1.0f),  glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(-1.0f,  0.0f,  0.0f),  glm::vec2(1.0f, 0.0f)},
        Vertex{glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3(-1.0f,  0.0f,  0.0f),  glm::vec2(1.0f, 1.0f)},
        Vertex{glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(-1.0f,  0.0f,  0.0f),  glm::vec2(0.0f, 1.0f)},
        Vertex{glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3(-1.0f,  0.0f,  0.0f),  glm::vec2(0.0f, 1.0f)}, // good
        Vertex{glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3(-1.0f,  0.0f,  0.0f),  glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(-1.0f,  0.0f,  0.0f),  glm::vec2(1.0f, 0.0f)},
        Vertex{glm::vec3( 0.5f,  0.5f,  0.5f),  glm::vec3( 1.0f,  0.0f,  0.0f),  glm::vec2(1.0f, 0.0f)},
        Vertex{glm::vec3( 0.5f, -0.5f, -0.5f),  glm::vec3( 1.0f,  0.0f,  0.0f),  glm::vec2(0.0f, 1.0f)},
        Vertex{glm::vec3( 0.5f,  0.5f, -0.5f),  glm::vec3( 1.0f,  0.0f,  0.0f),  glm::vec2(1.0f, 1.0f)},
        Vertex{glm::vec3( 0.5f, -0.5f, -0.5f),  glm::vec3( 1.0f,  0.0f,  0.0f),  glm::vec2(0.0f, 1.0f)}, // good
        Vertex{glm::vec3( 0.5f,  0.5f,  0.5f),  glm::vec3( 1.0f,  0.0f,  0.0f),  glm::vec2(1.0f, 0.0f)},
        Vertex{glm::vec3( 0.5f, -0.5f,  0.5f),  glm::vec3( 1.0f,  0.0f,  0.0f),  glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3( 0.0f, -1.0f,  0.0f),  glm::vec2(0.0f, 1.0f)},
        Vertex{glm::vec3( 0.5f, -0.5f, -0.5f),  glm::vec3( 0.0f, -1.0f,  0.0f),  glm::vec2(1.0f, 1.0f)},
        Vertex{glm::vec3( 0.5f, -0.5f,  0.5f),  glm::vec3( 0.0f, -1.0f,  0.0f),  glm::vec2(1.0f, 0.0f)},
        Vertex{glm::vec3( 0.5f, -0.5f,  0.5f),  glm::vec3( 0.0f, -1.0f,  0.0f),  glm::vec2(1.0f, 0.0f)},
        Vertex{glm::vec3(-0.5f, -0.5f,  0.5f),  glm::vec3( 0.0f, -1.0f,  0.0f),  glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3(-0.5f, -0.5f, -0.5f),  glm::vec3( 0.0f, -1.0f,  0.0f),  glm::vec2(0.0f, 1.0f)},
        Vertex{glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3( 0.0f,  1.0f,  0.0f),  glm::vec2(0.0f, 1.0f)},
        Vertex{glm::vec3( 0.5f,  0.5f,  0.5f),  glm::vec3( 0.0f,  1.0f,  0.0f),  glm::vec2(1.0f, 0.0f)},
        Vertex{glm::vec3( 0.5f,  0.5f, -0.5f),  glm::vec3( 0.0f,  1.0f,  0.0f),  glm::vec2(1.0f, 1.0f)},
        Vertex{glm::vec3( 0.5f,  0.5f,  0.5f),  glm::vec3( 0.0f,  1.0f,  0.0f),  glm::vec2(1.0f, 0.0f)},
        Vertex{glm::vec3(-0.5f,  0.5f, -0.5f),  glm::vec3( 0.0f,  1.0f,  0.0f),  glm::vec2(0.0f, 1.0f)},
        Vertex{glm::vec3(-0.5f,  0.5f,  0.5f),  glm::vec3(0.0f,  1.0f,  0.0f),   glm::vec2(0.0f, 0.0f)}
    };

    float QuadVertices[] = {   // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    int maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    std::cout << "Max number of samples for multisample MSAA framebuffer = " << maxSamples << "\n";
    int maxCompteWorkGroupx = 0, maxCompteWorkGroupy = 0, maxCompteWorkGroupz = 0;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxCompteWorkGroupx);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxCompteWorkGroupy);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxCompteWorkGroupz);
    std::cout << maxSamples << "\n";
    std::cout << maxCompteWorkGroupx << "\n";
    std::cout << maxCompteWorkGroupy << "\n";
    std::cout << maxCompteWorkGroupz << "\n";

    glm::vec3 pos1(-1.0f, 1.0f, 0.0f);
    glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
    glm::vec3 pos3(1.0f, -1.0f, 0.0f);
    glm::vec3 pos4(1.0f, 1.0f, 0.0f);
    // texture coordinates
    glm::vec2 uv1(0.0f, 1.0f);
    glm::vec2 uv2(0.0f, 0.0f);
    glm::vec2 uv3(1.0f, 0.0f);
    glm::vec2 uv4(1.0f, 1.0f);
    // normal vector
    glm::vec3 nm(0.0f, 0.0f, 1.0f);

    // calculate tangent/bitangent vectors of both triangles
    glm::vec3 tangent1{}, bitangent1{};
    glm::vec3 tangent2{}, bitangent2{};
    // triangle 1
    // ----------
    glm::vec3 edge1 = pos2 - pos1;
    glm::vec3 edge2 = pos3 - pos1;
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

    bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

    // triangle 2
    // ----------
    edge1 = pos3 - pos1;
    edge2 = pos4 - pos1;
    deltaUV1 = uv3 - uv1;
    deltaUV2 = uv4 - uv1;

    f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);


    bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

    //-------------------------------------------------------
    //Quad used for debugging and sometimes as a floor.
    float quadVertices[] = {
        // positions            // normal         // texcoords  // tangent                          // bitangent
        pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
        pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
        pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

        pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
        pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
        pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
    };
    unsigned int quadVAO = 0;
    unsigned int quadVBO;
    // configure plane VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    glBindVertexArray(0);

    //Textures Used in the floor.
    Texture BrickTexture;
    BrickTexture.LoadTexture("Textures/brickwall.jpg");
    BrickTexture.GenerateTexture(0, GL_REPEAT, true);
    Texture NormalTexture;
    NormalTexture.LoadTexture("Textures/brickwall_normal.jpg");
    NormalTexture.GenerateTexture(2, GL_REPEAT, true);
    NormalTexture.Bind(2);


    //------------------------------------------------------------------------------
    //skybox initialization
    unsigned int skybox2vao, skybox2vbo;
    glGenVertexArrays(1, &skybox2vao);
    glGenBuffers(1, &skybox2vbo);

    glBindVertexArray(skybox2vao);

    glBindBuffer(GL_ARRAY_BUFFER, skybox2vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int Boxvao, boxvbo;
    glGenVertexArrays(1, &Boxvao);
    glGenBuffers(1, &boxvbo);

    glBindVertexArray(Boxvao);

    glBindBuffer(GL_ARRAY_BUFFER, boxvbo);
    glBufferData(GL_ARRAY_BUFFER, Cubevertices.size() * sizeof(Vertex), Cubevertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));

    unsigned int QuadVao, QuadVbo;
    glGenVertexArrays(1, &QuadVao);
    glGenBuffers(1, &QuadVbo);
    glBindVertexArray(QuadVao);
    glBindBuffer(GL_ARRAY_BUFFER, QuadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), &QuadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    //----------------------------------------------------------------------
    //Normal skybox instead of the hdr skybox, uses this in the skybox if you want different types of skybox
    Texture skyboxtexture;
    skyboxtexture.loadCubemap(faces);

    //--------------------------------------------------------------------------------------------------
    //uniform buffer for view and projection matrix
    unsigned int uniformbuffer;
    glGenBuffers(1, &uniformbuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, uniformbuffer);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformbuffer, 0, 2 * sizeof(glm::mat4));

    // Our framebuffer quad
    unsigned int QuadVAO, QuadVBO;
    glGenVertexArrays(1, &QuadVAO);
    glGenBuffers(1, &QuadVBO);
    glBindVertexArray(QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), &QuadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    //-------------------------------------------------------------------------------------
    //Enable some Important stuff
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // enable seamless cubemap sampling

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // set depth function to less than AND equal for skybox depth trick.

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_MULTISAMPLE);

    //------------------------------------------------------------------------------------------------------
    //Applying MSAA, currently set to Maximum number of supported samples, change it according to your needs.
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a multisampled color attachment texture
    unsigned int textureColorBufferMultiSampled;
    glGenTextures(1, &textureColorBufferMultiSampled);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, maxSamples, GL_RGBA16F, Width, Height, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, maxSamples, GL_DEPTH24_STENCIL8, Width, Height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! MultiSampled \n" << fboStatus;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // configure second post-processing framebuffer
    unsigned int intermediateFBO;
    glGenFramebuffers(1, &intermediateFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
    // create a color attachment texture
    unsigned int screenTexture;
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Width, Height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, screenTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // depth buffer for Debugging purposes
    //Not Needed Right Now
    /*
    unsigned int Depth;
    glGenTextures(1, &Depth);
    glBindTexture(GL_TEXTURE_2D, Depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, Width, Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    unsigned int DepthBuffer;
    glGenFramebuffers(1, &DepthBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, DepthBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Depth, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
*/

    int drawBuffers = 0;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &drawBuffers);
    std::cout << "Max Draw Buffers: " << drawBuffers << std::endl;


    //----------------------------------------------------------------------------------------------------------------------------
    //FrameBuffer For capturing hdr cubemap background.
    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 2048, 2048);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    Texture hdrskyboxtex;
    hdrskyboxtex.loadHDREquiRectangularMap(hdrface);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
       glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        // note that we store each face with 16 bit floating point values
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
            2048, 2048, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    hdrskybox.use();
    hdrskybox.setInt("equirectangularMap", 0);
    hdrskybox.setMat4("projection", captureProjection);
    hdrskyboxtex.Bind(0);
    glViewport(0, 0, 2048, 2048); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    for (unsigned int i = 0; i < 6; ++i)
    {
        hdrskybox.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(skybox2vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    HdrCubeMap.use();
    HdrCubeMap.setInt("environmentMap", 0);

    Framebuffer.use();
    Framebuffer.setInt("text_diffuse1", 0);

    skybox.use();
    skybox.setInt("skybox", 0);

    glViewport(0, 0, Width, Height);

    std::vector<PointLight> pointlight;
    glm::vec3 lightPos(0.0f, 1.0f, -2.5f);
    glm::vec3 lightPos1(2.0f, 1.0f, 1.0f);
    glm::vec3 lightColors1(glm::vec4(glm::vec3(15.0f), 1.0f));
    glm::vec3 lightColors2(glm::vec4(5.0f, 0.0f, 0.0f ,1.0f));
    float constant = 1.0;
    float linear = 0.09f;
    float quadratic = 0.032f;

    std::vector<glm::vec3> LIGHTPOSITIONS;
    std::vector<glm::vec3> LIGHTCOLORS;

    for (unsigned int i = 0; i < 10; i++)
    {
        // calculate slightly random offsets
        float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
        float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
        float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
        LIGHTPOSITIONS.emplace_back(xPos, yPos, zPos);
        // also calculate random color
        float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
        float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
        float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
        LIGHTCOLORS.emplace_back(rColor, gColor, bColor);

        float lightMax = std::fmaxf(std::fmaxf(LIGHTCOLORS[i].r, LIGHTCOLORS[i].g), LIGHTCOLORS[i].b);
        float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * lightMax)))
    / (2 * quadratic);
        pointlight.emplace_back(glm::vec4(LIGHTPOSITIONS[i], 1.0f), glm::vec4(LIGHTCOLORS[i], 1.0f), 1.0f, radius);
    }

    unsigned int SSBOCLUSTER;
    glGenBuffers(1, &SSBOCLUSTER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBOCLUSTER);
    glBufferData(GL_SHADER_STORAGE_BUFFER, Engine::numclusters * sizeof(struct VolumeTileAABB), NULL, GL_STATIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SSBOCLUSTER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    unsigned int SSBOPointLight;
    glGenBuffers(1, &SSBOPointLight);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBOPointLight);
    glBufferData(GL_SHADER_STORAGE_BUFFER, pointlight.size() * sizeof(PointLight), pointlight.data(), GL_DYNAMIC_DRAW);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, SSBOPointLight);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // configure depth map FBO
// -----------------------
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    // create depth texture
    const int NR_LIGHTS = 2;
    unsigned int depthCubeMap;
    glGenTextures(1, &depthCubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, depthCubeMap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 32 * 6, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
    // attach depth texture as FBO's depth buffers
    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    float near_plane = 0.1f;
    float far_plane = 100.0f;

    ourShader.use();
    ourShader.setInt("texture_diffuse1", 0);
    ourShader.setInt("texture_specular1", 1);
    ourShader.setInt("texture_normal1", 2);
    ourShader.setInt("depthMap", 3);
    //ourShader.setInt("ParallaxMap", 4);

    glm::mat4 shadowProjection = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
    std::vector<glm::mat4> shadowTransforms;


    for (unsigned int i = 0; i < pointlight.size(); i++)
    {
        shadowTransforms.emplace_back(shadowProjection *
            glm::lookAt(glm::vec3(pointlight[i].position), glm::vec3(pointlight[i].position) + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.emplace_back(shadowProjection *
            glm::lookAt(glm::vec3(pointlight[i].position), glm::vec3(pointlight[i].position) + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.emplace_back(shadowProjection *
            glm::lookAt(glm::vec3(pointlight[i].position), glm::vec3(pointlight[i].position) + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms.emplace_back(shadowProjection *
            glm::lookAt(glm::vec3(pointlight[i].position), glm::vec3(pointlight[i].position) + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
        shadowTransforms.emplace_back(shadowProjection *
            glm::lookAt(glm::vec3(pointlight[i].position), glm::vec3(pointlight[i].position) + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.emplace_back(shadowProjection *
            glm::lookAt(glm::vec3(pointlight[i].position), glm::vec3(pointlight[i].position) + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));
    }

    // ---------------------------------------------------------
    // render loop variables.
    double xpos, ypos;
    int glfwwidth = 0, glfwheight = 0;

    // Setup Platform/Renderer backends
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        std::string title = "PiratesEngine - " + std::to_string(1 / deltaTime) + "FPS";
        glfwSetWindowTitle(window, title.c_str());
        glfwGetWindowSize(window, &glfwwidth, &glfwheight);
        glfwGetCursorPos(window, &xpos, &ypos);
        camera.ProcessKeyboard(window, deltaTime);
        camera.ProcessMouseMovement(window, xpos, ypos);

        if (glfwwidth != Width || glfwheight != Height) {
            camera.width = glfwwidth;
            camera.height = glfwheight;
            Width = glfwwidth;
            Height = glfwheight;
            glViewport(0, 0, Width, Height);

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            // create a multisampled color attachment texture
            //update multisampled texture
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, maxSamples, GL_RGBA16F, Width, Height, GL_TRUE);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, maxSamples, GL_DEPTH24_STENCIL8, Width, Height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
            glBindTexture(GL_TEXTURE_2D, screenTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, Width, Height, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            // attach texture to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "Framebuffer not complete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

        }
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glCullFace(GL_FRONT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        //for shadow rendering
        //a solution for this is to make a vector holding the matrices of each shadow and iterate through each shadow using it's index in the vector map!
        SimpleShader.use();
        for (unsigned int i = 0; i < pointlight.size(); i++) {
                for (unsigned int j = 0 + i * 6; j < 6 + i * 6; ++j) {
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0,  j);
                SimpleShader.setMat4("shadowMatrices", shadowTransforms[j]);
                SimpleShader.setFloat("far_plane", far_plane);

                SimpleShader.setInt("lightindex", i);
                SimpleShader.setVec3("lightPos", glm::vec3(pointlight[i].position));
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f)); // translate it down so it's at the center of the scene
                model = glm::scale(model, glm::vec3(0.01f));	// it's a bit too big for our scene, so scale it down
                model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                SimpleShader.setMat4("model", model);
                //Sponza.Draw(SimpleShader);

                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f)); // translate it down so it's at the center of the scene
                model = glm::rotate(model, glm::radians(0.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0))); // rotate the quad to show normal mapping from multiple directions
                model = glm::scale(model, glm::vec3(10.0f));	// it's a bit too big for our scene, so scale it down
                SimpleShader.setMat4("model", model);
                glBindVertexArray(quadVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-4.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
                model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0))); // rotate the quad to show normal mapping from multiple directions
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
                SimpleShader.setMat4("model", model);
                glBindVertexArray(quadVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);

            }

        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glCullFace(GL_BACK);
        glViewport(0, 0, Width, Height);

        view = camera.GetViewMatrix();
        projection = glm::perspective(glm::radians(45.0f), (float)Width / (float)Height, 1.0f, 100.0f);

        view = camera.GetViewMatrix();
        projection = glm::perspective(glm::radians(45.0f), (float)Width / (float)Height, 1.0f, 100.0f);

        //Forward+ rendering part
        //Generating Clustering and assigning them to their lights
        Clusters.use();

        Clusters.setFloat("zNear", zNear);
        Clusters.setFloat("zFar", zFar);
        Clusters.setUVec3("gridSize", glm::uvec3(Engine::gridX, Engine::gridY, Engine::gridZ));
        Clusters.setUVec2("screenDimensions", glm::uvec2(Width, Height));
        Clusters.setMat4("inverseProjection", glm::inverse(projection));

        glDispatchCompute(Engine::gridX, Engine::gridY ,Engine::gridZ);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        CullEngine.use();
        CullEngine.setMat4("viewMatrix", camera.GetViewMatrix());
        glDispatchCompute(27, 1 ,1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        ourShader.use();
        ourShader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setInt("numoflight", 2);
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setFloat("zFar", far_plane);
        ourShader.setFloat("zNear", near_plane);
        ourShader.setUVec3("gridSize", glm::uvec3(Engine::gridX, Engine::gridY, Engine::gridZ));
        ourShader.setUVec2("screenDimensions", glm::uvec2(Width, Height));

        //-------------------------------------------------------------------------------------------
        //Directional Light Setup
        ourShader.setVec3("dirlight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));

        ourShader.setVec3(("dirlight.ambient"), glm::vec3(0.5f));
        ourShader.setVec3("dirlight.specular", glm::vec3(0.3f));

        ourShader.setVec3("dirlight.diffuse", glm::vec3(5.0f));


        ourShader.setFloat("time", static_cast<float>(glfwGetTime()));

        //material specfication;
        ourShader.setVec3("material.ambient", glm::vec3(0.0f));
        ourShader.setFloat("material.shininess", 64.0f);
        projection = glm::perspective(glm::radians(45.0f), (float)Width / (float)Height, 1.0f, 100.0f);
        glBindBuffer(GL_UNIFORM_BUFFER, uniformbuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBuffer(GL_UNIFORM_BUFFER, uniformbuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.01f));	// it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        ourShader.setBool("normalMapON", true);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, depthCubeMap);
        Sponza.Draw(ourShader);

        glm::quat myfirstquat;
        float angle = glm::radians((float)glfwGetTime() * -1.0f);
        glm::vec3 axis = glm::vec3(1.0f, 0.0f, 0.0f);
        myfirstquat = glm::angleAxis(glm::degrees(angle), glm::normalize(axis));
        glm::mat4 rotation = glm::toMat4(myfirstquat);

        model = glm::mat4(1.0f);
        setmodeltranslationandscale(model, glm::vec3(1.0f), glm::vec3(2.0f, 2.0f, 1.0f), rotation);
        //model = glm::scale(model, glm::vec3(1.0f));
        //model = glm::translate(model, glm::vec3(2.0f, 2.0f, -1.0f));

        //model = glm::rotate(model, glm::radians(-45.0f), glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));	// it's a bit too big for our scene, so scale it down
        //model *= rotation;	// it's a bit too big for our scene, so scale it down

        ourShader.setBool("normalMapON", true);
        ourShader.setMat4("model", model);
        glDisable(GL_CULL_FACE);
        renderquad(quadVAO, BrickTexture, NormalTexture, depthCubeMap);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        /*
        glBindVertexArray(quadVAO);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-4.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(0.0, 1.0, 0.0))); // rotate the quad to show normal mapping from multiple directions
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        WoodToy.Bind(0);
        Brick2normalMap.Bind(2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, depthCubeMap);
        Parallaxmap.Bind(4);
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        */
        for (int i = 0; i < pointlight.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(pointlight[i].position));
            LightShader.use();
            LightShader.setMat4("view", view);
            LightShader.setMat4("projection", projection);
            LightShader.setMat4("model", model);
            LightShader.setVec4("LightColor", pointlight[i].Color);
            glBindVertexArray(Boxvao);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // convert HDR equirectangular environment map to cubemap equivalent
        glDepthFunc(GL_LEQUAL);
        skybox.use();
        skybox.setMat4("projection", projection);
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        hdrskybox.setMat4("view", view);
        glBindVertexArray(skybox2vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);

        glBlitFramebuffer(0, 0, Width, Height, 0, 0, Width, Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        Framebuffer.use();

        Framebuffer.setBool("gamma", true);
        Framebuffer.setBool("hdr", true);

        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        {
        Framebuffer.setBool("hdr", false);

        }
        Framebuffer.setFloat("exposure", 1.0f);
        Framebuffer.setFloat("gamma1", 1.2f);
        glBindVertexArray(QuadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screenTexture); // use the now resolved color attachment as the quad's texture
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_DEPTH_TEST);

        //Capturing Images.
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            GLsizei nrChannels = 3;
            GLsizei stride = nrChannels * width;
            GLsizei bufferSize = stride * height;
            std::vector<char> buffer(bufferSize);
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
            glReadBuffer(GL_BACK);
            glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
            stbi_flip_vertically_on_write(true);
            stbi_write_png("my_png_file.png", width, height, nrChannels, buffer.data(), stride);
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapInterval(1);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    Assimp::DefaultLogger::kill();
    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void renderquad(unsigned int quadVAO,Texture texture, Texture NormalTexture, unsigned int depthCubeMap)
{
    glBindVertexArray(quadVAO);
    texture.Bind(0);
    NormalTexture.Bind(2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, depthCubeMap);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
void setmodeltranslationandscale(glm::mat4& model, glm::vec3 scale, glm::vec3 translation, glm::mat4 rotation)
{
    model = glm::translate(model, translation);
    model *= rotation;
    model = glm::scale(model, scale);
}