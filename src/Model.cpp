#include "Model.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "../include/tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"
#include <GL/glut.h>
#include <iostream>
#include <map>
#include <cfloat>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Model::Model()
    : position(0.0f), scale(1.0f), rotation(0.0f),
      bbMin(0.0f), bbMax(0.0f), bbCenter(0.0f), maxDim(1.0f) {
}

Model::~Model() {
    meshes.clear();
}

bool Model::load(const std::string& filepath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Extract directory from filepath for material loading
    std::string base_dir = "./";
    size_t last_slash = filepath.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        base_dir = filepath.substr(0, last_slash + 1);
    }

    // Load OBJ with material search directory
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), base_dir.c_str());

    if (!warn.empty()) {
        std::cout << "Warning: " << warn << std::endl;
    }
    if (shapes.empty()) {
        std::cout << "Error: No shapes loaded" << std::endl;
        return false;
    }

    std::cout << "Loaded OBJ file: " << filepath << std::endl;
    std::cout << "Shapes: " << shapes.size() << ", Materials: " << materials.size() << std::endl;

    // Build material map
    std::map<std::string, Material> materialMap;
    for (const auto& mat : materials) {
        Material m;
        m.ambientColor  = glm::vec3(mat.ambient[0],  mat.ambient[1],  mat.ambient[2]);
        m.diffuseColor  = glm::vec3(mat.diffuse[0],  mat.diffuse[1],  mat.diffuse[2]);
        m.specularColor = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
        m.shininess     = mat.shininess;

        // Load diffuse texture if the .mtl references one
        if (!mat.diffuse_texname.empty()) {
            std::string texpath = mat.diffuse_texname;
            for (char& c : texpath) if (c == '\\') c = '/';  // fix Windows paths
            texpath = base_dir + texpath;

            int w, h, n;
            stbi_set_flip_vertically_on_load(true);
            unsigned char* data = stbi_load(texpath.c_str(), &w, &h, &n, 3);
            if (data) {
                GLuint tid;
                glGenTextures(1, &tid);
                glBindTexture(GL_TEXTURE_2D, tid);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                stbi_image_free(data);
                m.diffuseTexture = tid;
                std::cout << "Texture loaded: " << texpath << " (" << w << "x" << h << ")" << std::endl;
            } else {
                std::cout << "Texture missing: " << texpath << std::endl;
            }
        }

        materialMap[mat.name] = m;
    }

    // Process each shape
    for (size_t s = 0; s < shapes.size(); s++) {
        Mesh mesh;
        mesh.name = shapes[s].name;

        // Get material for this shape (use first material if available)
        if (!shapes[s].mesh.material_ids.empty() && shapes[s].mesh.material_ids[0] >= 0) {
            int mat_id = shapes[s].mesh.material_ids[0];
            if (mat_id < materials.size()) {
                mesh.materialName = materials[mat_id].name;
                mesh.material = materialMap[mesh.materialName];
            }
        }

        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                Vertex vertex;

                // Position
                vertex.position.x = attrib.vertices[3 * idx.vertex_index + 0];
                vertex.position.y = attrib.vertices[3 * idx.vertex_index + 1];
                vertex.position.z = attrib.vertices[3 * idx.vertex_index + 2];

                // Normal
                if (idx.normal_index >= 0) {
                    vertex.normal.x = attrib.normals[3 * idx.normal_index + 0];
                    vertex.normal.y = attrib.normals[3 * idx.normal_index + 1];
                    vertex.normal.z = attrib.normals[3 * idx.normal_index + 2];
                } else {
                    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                // Texture coordinate
                if (idx.texcoord_index >= 0) {
                    vertex.texCoord.x = attrib.texcoords[2 * idx.texcoord_index + 0];
                    vertex.texCoord.y = attrib.texcoords[2 * idx.texcoord_index + 1];
                } else {
                    vertex.texCoord = glm::vec2(0.0f, 0.0f);
                }

                mesh.vertices.push_back(vertex);
                mesh.indices.push_back(mesh.vertices.size() - 1);
            }
            index_offset += fv;
        }

        meshes.push_back(mesh);
        std::cout << "Mesh '" << mesh.name << "' loaded with " << mesh.vertices.size() << " vertices" << std::endl;
    }

    // Compute bounding box for normalisation
    bbMin = glm::vec3( FLT_MAX);
    bbMax = glm::vec3(-FLT_MAX);
    for (const auto& mesh : meshes)
        for (const auto& v : mesh.vertices) {
            bbMin = glm::min(bbMin, v.position);
            bbMax = glm::max(bbMax, v.position);
        }
    bbCenter = 0.5f * (bbMin + bbMax);
    glm::vec3 sz = bbMax - bbMin;
    maxDim = std::max({ sz.x, sz.y, sz.z });
    if (maxDim < 1e-6f) maxDim = 1.0f;
    std::cout << "BBox size=(" << sz.x << "," << sz.y << "," << sz.z
              << ") maxDim=" << maxDim << std::endl;

    std::cout << "Model loaded successfully!" << std::endl;
    return true;
}

void Model::render() {
    glPushMatrix();

    // World transform: position → rotation → desired size (in metres)
    glTranslatef(position.x, position.y, position.z);
    glRotatef(rotation.y, 0.0f, 1.0f, 0.0f);
    glRotatef(rotation.x, 1.0f, 0.0f, 0.0f);
    glRotatef(rotation.z, 0.0f, 0.0f, 1.0f);
    glScalef(scale.x, scale.y, scale.z);

    // Normalisation: fit model into a unit cube, base sits on y=0, x/z centred
    float k = 1.0f / maxDim;
    glScalef(k, k, k);
    glTranslatef(-bbCenter.x, -bbMin.y, -bbCenter.z);

    for (const auto& mesh : meshes)
        renderMesh(mesh);

    glPopMatrix();
}

void Model::renderMesh(const Mesh& mesh) {
    bool hasTexture = (mesh.material.diffuseTexture > 0);

    if (hasTexture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, mesh.material.diffuseTexture);
        glColor3f(1.0f, 1.0f, 1.0f);  // white so texture renders at full brightness
    } else {
        glDisable(GL_TEXTURE_2D);
        glm::vec3 d = mesh.material.diffuseColor;
        if (d.x + d.y + d.z < 0.05f) d = glm::vec3(0.75f);
        glColor3f(d.x, d.y, d.z);
    }

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        const Vertex& v = mesh.vertices[i];
        glNormal3f(v.normal.x, v.normal.y, v.normal.z);
        glTexCoord2f(v.texCoord.x, v.texCoord.y);
        glVertex3f(v.position.x, v.position.y, v.position.z);
    }
    glEnd();

    if (hasTexture) glDisable(GL_TEXTURE_2D);
}

void Model::setPosition(const glm::vec3& pos) {
    position = pos;
}

void Model::setScale(const glm::vec3& s) {
    scale = s;
}

void Model::setRotation(const glm::vec3& rot) {
    rotation = rot;
}
