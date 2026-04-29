#ifndef ASSIMP_MODEL_H
#define ASSIMP_MODEL_H

#include <string>
#include <vector>
#include <GL/glut.h>
#include <glm/glm.hpp>

class AssimpModel {
public:
    AssimpModel();
    ~AssimpModel();

    bool load(const std::string& path);
    void render();

    void setPosition(const glm::vec3& p) { position = p; }
    void setScale(const glm::vec3& s)    { scale = s; }
    void setRotation(const glm::vec3& r) { rotation = r; }
    glm::vec3 getPosition() const { return position; }

    int  vertexCount() const;
    int  meshCount()   const { return (int)meshes.size(); }

private:
    struct Vert {
        float x, y, z;
        float nx, ny, nz;
        float u, v;
    };

    struct AMesh {
        std::vector<Vert>         verts;
        std::vector<unsigned int> indices;
        glm::vec3  diffuse  = glm::vec3(0.75f);
        GLuint     texId    = 0;
    };

    std::vector<AMesh>  meshes;
    std::vector<GLuint> embeddedTexIds;   // textures decoded from the file itself

    glm::vec3 position = glm::vec3(0.f);
    glm::vec3 scale    = glm::vec3(1.f);
    glm::vec3 rotation = glm::vec3(0.f);

    glm::vec3 bbMin, bbMax, bbCenter;
    float     maxDim = 1.f;

    GLuint resolveTexture(const std::string& path, const std::string& baseDir,
                          const void* scene);   // void* avoids Assimp header in .h
};

#endif
