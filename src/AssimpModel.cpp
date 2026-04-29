#include "AssimpModel.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
// stb_image implementation is already compiled in Model.cpp; just declare here
#include "../include/stb_image.h"
#include <GL/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cfloat>
#include <algorithm>
#include <map>

// ── helpers ──────────────────────────────────────────────────────────────────
static glm::vec3 toGlm(const aiColor3D& c) { return glm::vec3(c.r, c.g, c.b); }

static GLuint uploadTexture(const unsigned char* data, int w, int h) {
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

// ── ctor / dtor ───────────────────────────────────────────────────────────────
AssimpModel::AssimpModel()
    : bbMin(FLT_MAX), bbMax(-FLT_MAX), bbCenter(0), maxDim(1) {}

AssimpModel::~AssimpModel() {
    for (GLuint id : embeddedTexIds) glDeleteTextures(1, &id);
    for (auto& m : meshes) if (m.texId) glDeleteTextures(1, &m.texId);
}

// ── load ──────────────────────────────────────────────────────────────────────
bool AssimpModel::load(const std::string& path) {
    Assimp::Importer imp;
    const aiScene* scene = imp.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices);

    if (!scene || !scene->mRootNode ||
        (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        std::cerr << "AssimpModel: " << imp.GetErrorString() << std::endl;
        return false;
    }

    std::string baseDir;
    size_t sl = path.find_last_of("/\\");
    if (sl != std::string::npos) baseDir = path.substr(0, sl + 1);

    // ── decode embedded textures ─────────────────────────────────────────────
    embeddedTexIds.resize(scene->mNumTextures, 0);
    for (unsigned int i = 0; i < scene->mNumTextures; ++i) {
        aiTexture* t = scene->mTextures[i];
        int w, h, n;
        unsigned char* data = nullptr;
        if (t->mHeight == 0) {
            // compressed blob (PNG/JPEG etc.)
            stbi_set_flip_vertically_on_load(false);
            data = stbi_load_from_memory(
                reinterpret_cast<unsigned char*>(t->pcData),
                t->mWidth, &w, &h, &n, 3);
        } else {
            // raw ARGB8888 — convert to RGB
            w = t->mWidth; h = t->mHeight;
            data = new unsigned char[w * h * 3];
            for (int p = 0; p < w * h; ++p) {
                data[p*3+0] = t->pcData[p].r;
                data[p*3+1] = t->pcData[p].g;
                data[p*3+2] = t->pcData[p].b;
            }
        }
        if (data) {
            embeddedTexIds[i] = uploadTexture(data, w, h);
            stbi_image_free(data);
        }
    }

    // ── materials ─────────────────────────────────────────────────────────────
    std::vector<glm::vec3> matDiffuse(scene->mNumMaterials, glm::vec3(0.75f));
    std::vector<GLuint>    matTex(scene->mNumMaterials, 0);

    for (unsigned int mi = 0; mi < scene->mNumMaterials; ++mi) {
        aiMaterial* mat = scene->mMaterials[mi];

        aiColor3D col(0.75f, 0.75f, 0.75f);
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, col) == AI_SUCCESS)
            matDiffuse[mi] = toGlm(col);

        // fall back to ambient if diffuse is black
        if (matDiffuse[mi].r + matDiffuse[mi].g + matDiffuse[mi].b < 0.05f) {
            aiColor3D amb(0.5f,0.5f,0.5f);
            if (mat->Get(AI_MATKEY_COLOR_AMBIENT, amb) == AI_SUCCESS)
                matDiffuse[mi] = toGlm(amb);
        }
        if (matDiffuse[mi].r + matDiffuse[mi].g + matDiffuse[mi].b < 0.05f)
            matDiffuse[mi] = glm::vec3(0.6f);

        // texture
        aiString texPath;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            std::string tp(texPath.C_Str());
            if (tp[0] == '*') {
                // embedded texture index
                int idx = std::stoi(tp.substr(1));
                if (idx < (int)embeddedTexIds.size())
                    matTex[mi] = embeddedTexIds[idx];
            } else {
                // external file
                for (char& c : tp) if (c == '\\') c = '/';
                std::string full = baseDir + tp;
                int w, h, n;
                stbi_set_flip_vertically_on_load(true);
                unsigned char* data = stbi_load(full.c_str(), &w, &h, &n, 3);
                if (data) {
                    matTex[mi] = uploadTexture(data, w, h);
                    stbi_image_free(data);
                }
            }
        }
    }

    // ── meshes ────────────────────────────────────────────────────────────────
    bbMin = glm::vec3( FLT_MAX);
    bbMax = glm::vec3(-FLT_MAX);

    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
        aiMesh* aim = scene->mMeshes[mi];
        AMesh mesh;
        mesh.diffuse = matDiffuse[aim->mMaterialIndex];
        mesh.texId   = matTex[aim->mMaterialIndex];

        for (unsigned int vi = 0; vi < aim->mNumVertices; ++vi) {
            Vert v;
            v.x = aim->mVertices[vi].x;
            v.y = aim->mVertices[vi].y;
            v.z = aim->mVertices[vi].z;
            bbMin = glm::min(bbMin, glm::vec3(v.x, v.y, v.z));
            bbMax = glm::max(bbMax, glm::vec3(v.x, v.y, v.z));

            v.nx = aim->mNormals ? aim->mNormals[vi].x : 0.f;
            v.ny = aim->mNormals ? aim->mNormals[vi].y : 1.f;
            v.nz = aim->mNormals ? aim->mNormals[vi].z : 0.f;

            if (aim->mTextureCoords[0]) {
                v.u = aim->mTextureCoords[0][vi].x;
                v.v = aim->mTextureCoords[0][vi].y;
            } else { v.u = 0.f; v.v = 0.f; }

            mesh.verts.push_back(v);
        }

        for (unsigned int fi = 0; fi < aim->mNumFaces; ++fi) {
            aiFace& face = aim->mFaces[fi];
            for (unsigned int k = 0; k < face.mNumIndices; ++k)
                mesh.indices.push_back(face.mIndices[k]);
        }

        meshes.push_back(std::move(mesh));
    }

    bbCenter = 0.5f * (bbMin + bbMax);
    glm::vec3 sz = bbMax - bbMin;
    maxDim = std::max({sz.x, sz.y, sz.z});
    if (maxDim < 1e-6f) maxDim = 1.f;

    std::cout << "AssimpModel loaded: " << path
              << "  meshes=" << meshes.size()
              << "  verts=" << vertexCount()
              << "  embeddedTex=" << embeddedTexIds.size() << std::endl;
    return true;
}

// ── render ────────────────────────────────────────────────────────────────────
void AssimpModel::render() {
    glPushMatrix();

    glTranslatef(position.x, position.y, position.z);
    glRotatef(rotation.y, 0, 1, 0);
    glRotatef(rotation.x, 1, 0, 0);
    glRotatef(rotation.z, 0, 0, 1);
    glScalef(scale.x, scale.y, scale.z);

    // normalise to unit cube, base at y=0, centred in x/z
    float k = 1.f / maxDim;
    glScalef(k, k, k);
    glTranslatef(-bbCenter.x, -bbMin.y, -bbCenter.z);

    for (const auto& mesh : meshes) {
        bool hasTex = (mesh.texId != 0);
        if (hasTex) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, mesh.texId);
            glColor3f(1, 1, 1);
        } else {
            glDisable(GL_TEXTURE_2D);
            glColor3f(mesh.diffuse.r, mesh.diffuse.g, mesh.diffuse.b);
        }

        glBegin(GL_TRIANGLES);
        for (unsigned int idx : mesh.indices) {
            const Vert& v = mesh.verts[idx];
            glNormal3f(v.nx, v.ny, v.nz);
            if (hasTex) glTexCoord2f(v.u, v.v);
            glVertex3f(v.x, v.y, v.z);
        }
        glEnd();

        if (hasTex) glDisable(GL_TEXTURE_2D);
    }

    glPopMatrix();
}

int AssimpModel::vertexCount() const {
    int n = 0;
    for (auto& m : meshes) n += (int)m.verts.size();
    return n;
}

GLuint AssimpModel::resolveTexture(const std::string&, const std::string&, const void*) {
    return 0; // placeholder – logic is inline in load()
}
