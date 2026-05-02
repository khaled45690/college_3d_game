#ifndef ASSIMP_MODEL_H
#define ASSIMP_MODEL_H

#include <string>
#include <vector>
#include <map>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class AssimpModel {
public:
    AssimpModel();
    ~AssimpModel();

    bool load(const std::string& path);

    // Load an animation-only FBX (same skeleton, different clip).
    // clipName is the key used in setAnimation().
    bool loadAnimation(const std::string& path, const std::string& clipName);

    // Switch the active animation clip. Pass "" to stop animating.
    void setAnimation(const std::string& clipName);

    // Advance animation time and recompute skinned vertices.
    void update(float dt);

    void render();

    void setPosition(const glm::vec3& p) { position = p; }
    void setScale(const glm::vec3& s)    { scale = s; }
    void setRotation(const glm::vec3& r) { rotation = r; }
    glm::vec3 getPosition() const { return position; }

    int vertexCount() const;
    int meshCount()   const { return (int)meshes.size(); }

private:
    // ── per-vertex data ───────────────────────────────────────────────────
    static constexpr int MAX_BONE_INFLUENCE = 4;

    struct Vert {
        float x, y, z;
        float nx, ny, nz;
        float u, v;
        int   boneIds[MAX_BONE_INFLUENCE];
        float weights[MAX_BONE_INFLUENCE];
    };

    struct SkinnedVert {   // CPU-skinned output (position + normal only)
        float x, y, z;
        float nx, ny, nz;
    };

    struct AMesh {
        std::vector<Vert>         verts;
        std::vector<SkinnedVert>  skinned;   // updated each frame when animated
        std::vector<unsigned int> indices;
        glm::vec3 diffuse = glm::vec3(0.75f);
        GLuint    texId   = 0;
    };

    // ── skeleton ──────────────────────────────────────────────────────────
    struct BoneInfo {
        glm::mat4 offsetMatrix   = glm::mat4(1.f);
        glm::mat4 finalTransform = glm::mat4(1.f);
    };

    struct SceneNode {
        std::string      name;
        glm::mat4        localTransform;
        int              parent = -1;
        std::vector<int> children;
    };

    // ── animation keyframes ───────────────────────────────────────────────
    struct VecKey  { double time; glm::vec3 value; };
    struct QuatKey { double time; glm::quat value; };

    struct NodeAnim {
        std::string           nodeName;
        std::vector<VecKey>   posKeys;
        std::vector<QuatKey>  rotKeys;
        std::vector<VecKey>   scaleKeys;
    };

    struct AnimClip {
        std::string                name;
        double                     duration    = 0.0;
        double                     ticksPerSec = 25.0;
        std::vector<NodeAnim>      channels;
        std::map<std::string, int> channelMap;
    };

    // ── data ──────────────────────────────────────────────────────────────
    std::vector<AMesh>    meshes;
    std::vector<GLuint>   embeddedTexIds;

    std::vector<BoneInfo>              bones;
    std::map<std::string, int>         boneMap;
    std::vector<SceneNode>             nodes;
    std::map<std::string, int>         nodeMap;
    glm::mat4                          globalInverseTransform = glm::mat4(1.f);

    std::vector<AnimClip>              clips;
    std::map<std::string, int>         clipMap;
    int                                activeClip = -1;
    double                             animTime   = 0.0;   // ticks
    std::string                        rootMotionNode;     // XZ translation stripped here

    glm::vec3 position = glm::vec3(0.f);
    glm::vec3 scale    = glm::vec3(1.f);
    glm::vec3 rotation = glm::vec3(0.f);

    glm::vec3 bbMin, bbMax, bbCenter;
    float     maxDim = 1.f;

    // ── helpers ───────────────────────────────────────────────────────────
    void buildNodeTree(const void* aiNodePtr, int parentIdx);
    void loadAnimClip(const void* aiAnimPtr, const std::string& name);
    void computeSkinning();
    void updateNode(int nodeIdx, const glm::mat4& parentGlobal,
                    double time, const AnimClip& clip);

    glm::vec3 interpVec (const std::vector<VecKey>&  keys, double time) const;
    glm::quat interpQuat(const std::vector<QuatKey>& keys, double time) const;

    GLuint uploadTexture(const unsigned char* data, int w, int h);
    GLuint resolveExternalTexture(const std::string& texPath,
                                  const std::string& baseDir);
};

#endif
