#define GLM_ENABLE_EXPERIMENTAL
#include "AssimpModel.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../include/stb_image.h"
#include <GL/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <cfloat>
#include <algorithm>

// ── matrix conversion (Assimp row-major → GLM column-major) ──────────────────
static glm::mat4 toGlm(const aiMatrix4x4& m) {
    glm::mat4 r;
    r[0][0]=m.a1; r[1][0]=m.a2; r[2][0]=m.a3; r[3][0]=m.a4;
    r[0][1]=m.b1; r[1][1]=m.b2; r[2][1]=m.b3; r[3][1]=m.b4;
    r[0][2]=m.c1; r[1][2]=m.c2; r[2][2]=m.c3; r[3][2]=m.c4;
    r[0][3]=m.d1; r[1][3]=m.d2; r[2][3]=m.d3; r[3][3]=m.d4;
    return r;
}
static glm::vec3 toGlm(const aiColor3D& c)  { return {c.r, c.g, c.b}; }
static glm::vec3 toGlm(const aiVector3D& v) { return {v.x, v.y, v.z}; }
static glm::quat toGlm(const aiQuaternion& q) { return {q.w, q.x, q.y, q.z}; }

// ── ctor / dtor ───────────────────────────────────────────────────────────────
AssimpModel::AssimpModel()
    : bbMin(FLT_MAX), bbMax(-FLT_MAX), bbCenter(0), maxDim(1) {}

AssimpModel::~AssimpModel() {
    for (GLuint id : embeddedTexIds) glDeleteTextures(1, &id);
    for (auto& m : meshes) if (m.texId) glDeleteTextures(1, &m.texId);
}

// ── texture helpers ───────────────────────────────────────────────────────────
GLuint AssimpModel::uploadTexture(const unsigned char* data, int w, int h) {
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

GLuint AssimpModel::resolveExternalTexture(const std::string& texPath,
                                            const std::string& baseDir) {
    std::string tp = texPath;
    for (char& c : tp) if (c == '\\') c = '/';
    std::string full = baseDir + tp;
    int w, h, n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(full.c_str(), &w, &h, &n, 3);
    if (!data) return 0;
    GLuint id = uploadTexture(data, w, h);
    stbi_image_free(data);
    return id;
}

// ── node tree ─────────────────────────────────────────────────────────────────
void AssimpModel::buildNodeTree(const void* aiNodePtr, int parentIdx) {
    const aiNode* node = reinterpret_cast<const aiNode*>(aiNodePtr);
    SceneNode sn;
    sn.name           = node->mName.C_Str();
    sn.localTransform = toGlm(node->mTransformation);
    sn.parent         = parentIdx;

    int myIdx = (int)nodes.size();
    nodes.push_back(sn);
    nodeMap[sn.name] = myIdx;

    if (parentIdx >= 0)
        nodes[parentIdx].children.push_back(myIdx);

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        buildNodeTree(node->mChildren[i], myIdx);
}

// ── animation clip loading ────────────────────────────────────────────────────
void AssimpModel::loadAnimClip(const void* aiAnimPtr, const std::string& name) {
    const aiAnimation* anim = reinterpret_cast<const aiAnimation*>(aiAnimPtr);
    AnimClip clip;
    clip.name        = name;
    clip.duration    = anim->mDuration;
    clip.ticksPerSec = anim->mTicksPerSecond > 0.0 ? anim->mTicksPerSecond : 25.0;

    for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
        const aiNodeAnim* ch = anim->mChannels[c];
        NodeAnim na;
        na.nodeName = ch->mNodeName.C_Str();

        for (unsigned int k = 0; k < ch->mNumPositionKeys; ++k)
            na.posKeys.push_back({ch->mPositionKeys[k].mTime,
                                  toGlm(ch->mPositionKeys[k].mValue)});
        for (unsigned int k = 0; k < ch->mNumRotationKeys; ++k)
            na.rotKeys.push_back({ch->mRotationKeys[k].mTime,
                                  toGlm(ch->mRotationKeys[k].mValue)});
        for (unsigned int k = 0; k < ch->mNumScalingKeys; ++k)
            na.scaleKeys.push_back({ch->mScalingKeys[k].mTime,
                                    toGlm(ch->mScalingKeys[k].mValue)});

        int idx = (int)clip.channels.size();
        clip.channelMap[na.nodeName] = idx;
        clip.channels.push_back(std::move(na));
    }

    clipMap[name] = (int)clips.size();
    clips.push_back(std::move(clip));
}

// ── load (geometry + skeleton + embedded anims) ───────────────────────────────
bool AssimpModel::load(const std::string& path) {
    Assimp::Importer imp;
    const aiScene* scene = imp.ReadFile(path,
        aiProcess_Triangulate       |
        aiProcess_GenSmoothNormals  |
        aiProcess_FlipUVs           |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights  |
        aiProcess_CalcTangentSpace);

    if (!scene || !scene->mRootNode ||
        (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        std::cerr << "AssimpModel::load: " << imp.GetErrorString() << "\n";
        return false;
    }

    std::string baseDir;
    size_t sl = path.find_last_of("/\\");
    if (sl != std::string::npos) baseDir = path.substr(0, sl + 1);

    // ── global inverse transform ──────────────────────────────────────────
    aiMatrix4x4 rootTf = scene->mRootNode->mTransformation;
    rootTf.Inverse();
    globalInverseTransform = toGlm(rootTf);

    // ── embedded textures ─────────────────────────────────────────────────
    embeddedTexIds.resize(scene->mNumTextures, 0);
    for (unsigned int i = 0; i < scene->mNumTextures; ++i) {
        aiTexture* t = scene->mTextures[i];
        int w, h, n;
        unsigned char* data = nullptr;
        if (t->mHeight == 0) {
            stbi_set_flip_vertically_on_load(false);
            data = stbi_load_from_memory(
                reinterpret_cast<unsigned char*>(t->pcData),
                (int)t->mWidth, &w, &h, &n, 3);
        } else {
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

    // ── materials ─────────────────────────────────────────────────────────
    std::vector<glm::vec3> matDiffuse(scene->mNumMaterials, glm::vec3(0.75f));
    std::vector<GLuint>    matTex(scene->mNumMaterials, 0);

    for (unsigned int mi = 0; mi < scene->mNumMaterials; ++mi) {
        aiMaterial* mat = scene->mMaterials[mi];

        aiColor3D col(0.75f, 0.75f, 0.75f);
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, col) == AI_SUCCESS)
            matDiffuse[mi] = toGlm(col);
        if (matDiffuse[mi].r + matDiffuse[mi].g + matDiffuse[mi].b < 0.05f) {
            aiColor3D amb(0.5f,0.5f,0.5f);
            if (mat->Get(AI_MATKEY_COLOR_AMBIENT, amb) == AI_SUCCESS)
                matDiffuse[mi] = toGlm(amb);
        }
        if (matDiffuse[mi].r + matDiffuse[mi].g + matDiffuse[mi].b < 0.05f)
            matDiffuse[mi] = glm::vec3(0.6f);

        aiString texPath;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            std::string tp(texPath.C_Str());
            if (tp[0] == '*') {
                int idx = std::stoi(tp.substr(1));
                if (idx < (int)embeddedTexIds.size())
                    matTex[mi] = embeddedTexIds[idx];
            } else {
                matTex[mi] = resolveExternalTexture(tp, baseDir);
            }
        }
    }

    // ── meshes + bone weights ─────────────────────────────────────────────
    bbMin = glm::vec3( FLT_MAX);
    bbMax = glm::vec3(-FLT_MAX);

    for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
        aiMesh* aim = scene->mMeshes[mi];
        AMesh mesh;
        mesh.diffuse = matDiffuse[aim->mMaterialIndex];
        mesh.texId   = matTex[aim->mMaterialIndex];

        for (unsigned int vi = 0; vi < aim->mNumVertices; ++vi) {
            Vert v{};
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
            }
            mesh.verts.push_back(v);
        }

        // bone weights
        for (unsigned int bi = 0; bi < aim->mNumBones; ++bi) {
            aiBone* bone = aim->mBones[bi];
            std::string boneName = bone->mName.C_Str();

            int boneIdx;
            auto it = boneMap.find(boneName);
            if (it == boneMap.end()) {
                boneIdx = (int)bones.size();
                BoneInfo info;
                info.offsetMatrix = toGlm(bone->mOffsetMatrix);
                bones.push_back(info);
                boneMap[boneName] = boneIdx;
            } else {
                boneIdx = it->second;
            }

            for (unsigned int wi = 0; wi < bone->mNumWeights; ++wi) {
                unsigned int vertIdx = bone->mWeights[wi].mVertexId;
                float        weight  = bone->mWeights[wi].mWeight;
                // store in first free slot (Assimp limits to 4 with LimitBoneWeights)
                Vert& vt = mesh.verts[vertIdx];
                for (int s = 0; s < MAX_BONE_INFLUENCE; ++s) {
                    if (vt.weights[s] == 0.f) {
                        vt.boneIds[s] = boneIdx;
                        vt.weights[s] = weight;
                        break;
                    }
                }
            }
        }

        for (unsigned int fi = 0; fi < aim->mNumFaces; ++fi)
            for (unsigned int k = 0; k < aim->mFaces[fi].mNumIndices; ++k)
                mesh.indices.push_back(aim->mFaces[fi].mIndices[k]);

        // initialise skinned buffer as copy of rest pose
        mesh.skinned.resize(mesh.verts.size());
        for (size_t vi = 0; vi < mesh.verts.size(); ++vi) {
            mesh.skinned[vi] = {mesh.verts[vi].x, mesh.verts[vi].y, mesh.verts[vi].z,
                                mesh.verts[vi].nx, mesh.verts[vi].ny, mesh.verts[vi].nz};
        }

        meshes.push_back(std::move(mesh));
    }

    bbCenter = 0.5f * (bbMin + bbMax);
    glm::vec3 sz = bbMax - bbMin;
    maxDim = std::max({sz.x, sz.y, sz.z});
    if (maxDim < 1e-6f) maxDim = 1.f;

    // ── node hierarchy ────────────────────────────────────────────────────
    buildNodeTree(scene->mRootNode, -1);

    // ── animations embedded in this file ──────────────────────────────────
    for (unsigned int ai = 0; ai < scene->mNumAnimations; ++ai) {
        std::string clipName = scene->mAnimations[ai]->mName.C_Str();
        if (clipName.empty()) clipName = "anim_" + std::to_string(ai);
        loadAnimClip(scene->mAnimations[ai], clipName);
    }

    std::cout << "AssimpModel loaded: " << path
              << "  meshes=" << meshes.size()
              << "  bones=" << bones.size()
              << "  clips=" << clips.size() << "\n";
    return true;
}

// ── loadAnimation (separate FBX with only animation data) ────────────────────
bool AssimpModel::loadAnimation(const std::string& path,
                                 const std::string& clipName) {
    Assimp::Importer imp;
    const aiScene* scene = imp.ReadFile(path,
        aiProcess_Triangulate | aiProcess_LimitBoneWeights);
    if (!scene || !scene->mRootNode) {
        std::cerr << "AssimpModel::loadAnimation: " << imp.GetErrorString() << "\n";
        return false;
    }
    if (scene->mNumAnimations == 0) {
        std::cerr << "AssimpModel::loadAnimation: no animations in " << path << "\n";
        return false;
    }
    loadAnimClip(scene->mAnimations[0], clipName);
    std::cout << "Loaded animation '" << clipName << "' from " << path << "\n";
    return true;
}

// ── setAnimation ─────────────────────────────────────────────────────────────
void AssimpModel::setAnimation(const std::string& clipName) {
    if (clipName.empty()) { activeClip = -1; return; }
    auto it = clipMap.find(clipName);
    if (it == clipMap.end()) return;
    if (activeClip == it->second) return;   // already playing
    activeClip = it->second;
    animTime   = 0.0;

    // find root motion node: first node (top-down) that has animation channels
    rootMotionNode.clear();
    const AnimClip& clip = clips[activeClip];
    for (const SceneNode& sn : nodes) {
        if (clip.channelMap.count(sn.name)) {
            rootMotionNode = sn.name;
            break;
        }
    }
}

// ── interpolation helpers ─────────────────────────────────────────────────────
glm::vec3 AssimpModel::interpVec(const std::vector<VecKey>& keys,
                                  double time) const {
    if (keys.empty())  return glm::vec3(0.f);
    if (keys.size()==1) return keys[0].value;
    // find the two surrounding keys
    int idx = 0;
    for (int i = 0; i < (int)keys.size()-1; ++i) {
        if (time < keys[i+1].time) { idx = i; break; }
        idx = i;
    }
    if (idx >= (int)keys.size()-1) return keys.back().value;
    double dt = keys[idx+1].time - keys[idx].time;
    float  t  = (dt > 1e-9) ? (float)((time - keys[idx].time) / dt) : 0.f;
    t = glm::clamp(t, 0.f, 1.f);
    return glm::mix(keys[idx].value, keys[idx+1].value, t);
}

glm::quat AssimpModel::interpQuat(const std::vector<QuatKey>& keys,
                                   double time) const {
    if (keys.empty())   return glm::quat(1,0,0,0);
    if (keys.size()==1) return keys[0].value;
    int idx = 0;
    for (int i = 0; i < (int)keys.size()-1; ++i) {
        if (time < keys[i+1].time) { idx = i; break; }
        idx = i;
    }
    if (idx >= (int)keys.size()-1) return keys.back().value;
    double dt = keys[idx+1].time - keys[idx].time;
    float  t  = (dt > 1e-9) ? (float)((time - keys[idx].time) / dt) : 0.f;
    t = glm::clamp(t, 0.f, 1.f);
    return glm::slerp(keys[idx].value, keys[idx+1].value, t);
}

// ── compute bone transforms by traversing node hierarchy ─────────────────────
void AssimpModel::updateNode(int nodeIdx, const glm::mat4& parentGlobal,
                              double time, const AnimClip& clip) {
    const SceneNode& sn = nodes[nodeIdx];
    glm::mat4 local = sn.localTransform;

    auto it = clip.channelMap.find(sn.name);
    if (it != clip.channelMap.end()) {
        const NodeAnim& na = clip.channels[it->second];
        glm::vec3 pos   = interpVec (na.posKeys,   time);
        glm::quat rot   = interpQuat(na.rotKeys,   time);
        glm::vec3 scl   = interpVec (na.scaleKeys, time);
        if (na.scaleKeys.empty()) scl = glm::vec3(1.f);

        // strip root motion: the game drives world position, not the animation
        if (sn.name == rootMotionNode) {
            pos.x = 0.f;
            pos.z = 0.f;
        }

        glm::mat4 T = glm::translate(glm::mat4(1.f), pos);
        glm::mat4 R = glm::toMat4(rot);
        glm::mat4 S = glm::scale(glm::mat4(1.f), scl);
        local = T * R * S;
    }

    glm::mat4 global = parentGlobal * local;

    auto boneIt = boneMap.find(sn.name);
    if (boneIt != boneMap.end()) {
        int bi = boneIt->second;
        bones[bi].finalTransform =
            globalInverseTransform * global * bones[bi].offsetMatrix;
    }

    for (int child : sn.children)
        updateNode(child, global, time, clip);
}

// ── CPU skinning pass ─────────────────────────────────────────────────────────
void AssimpModel::computeSkinning() {
    for (auto& mesh : meshes) {
        for (size_t vi = 0; vi < mesh.verts.size(); ++vi) {
            const Vert& v = mesh.verts[vi];
            glm::vec4 pos(0.f), nor(0.f);
            float totalW = 0.f;

            for (int s = 0; s < MAX_BONE_INFLUENCE; ++s) {
                float w = v.weights[s];
                if (w == 0.f) continue;
                totalW += w;
                const glm::mat4& bm = bones[v.boneIds[s]].finalTransform;
                pos += w * (bm * glm::vec4(v.x,  v.y,  v.z,  1.f));
                nor += w * (bm * glm::vec4(v.nx, v.ny, v.nz, 0.f));
            }

            if (totalW < 1e-6f) {
                // no bone influence — use rest pose
                mesh.skinned[vi] = {v.x, v.y, v.z, v.nx, v.ny, v.nz};
            } else {
                glm::vec3 n = glm::normalize(glm::vec3(nor));
                mesh.skinned[vi] = {pos.x, pos.y, pos.z, n.x, n.y, n.z};
            }
        }
    }
}

// ── update (advance time, skin) ───────────────────────────────────────────────
void AssimpModel::update(float dt) {
    if (activeClip < 0 || bones.empty() || nodes.empty()) return;
    const AnimClip& clip = clips[activeClip];
    animTime += dt * clip.ticksPerSec;
    if (clip.duration > 0.0)
        animTime = std::fmod(animTime, clip.duration);

    // reset bone finals
    for (auto& b : bones) b.finalTransform = glm::mat4(1.f);

    // traverse node tree from root (index 0)
    updateNode(0, glm::mat4(1.f), animTime, clip);
    computeSkinning();
}

// ── render ────────────────────────────────────────────────────────────────────
void AssimpModel::render() {
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    glRotatef(rotation.y, 0, 1, 0);
    glRotatef(rotation.x, 1, 0, 0);
    glRotatef(rotation.z, 0, 0, 1);
    glScalef(scale.x, scale.y, scale.z);
    float k = 1.f / maxDim;
    glScalef(k, k, k);
    glTranslatef(-bbCenter.x, -bbMin.y, -bbCenter.z);

    bool animated = (activeClip >= 0 && !bones.empty());

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
        if (animated) {
            for (unsigned int idx : mesh.indices) {
                const SkinnedVert& sv = mesh.skinned[idx];
                const Vert&         v  = mesh.verts[idx];
                glNormal3f(sv.nx, sv.ny, sv.nz);
                if (hasTex) glTexCoord2f(v.u, v.v);
                glVertex3f(sv.x,  sv.y,  sv.z);
            }
        } else {
            for (unsigned int idx : mesh.indices) {
                const Vert& v = mesh.verts[idx];
                glNormal3f(v.nx, v.ny, v.nz);
                if (hasTex) glTexCoord2f(v.u, v.v);
                glVertex3f(v.x,  v.y,  v.z);
            }
        }
        glEnd();

        if (hasTex) glDisable(GL_TEXTURE_2D);
    }

    glPopMatrix();
}

// ── vertexCount ───────────────────────────────────────────────────────────────
int AssimpModel::vertexCount() const {
    int n = 0;
    for (auto& m : meshes) n += (int)m.verts.size();
    return n;
}
