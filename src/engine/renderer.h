#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

namespace engine {

/// GPU data for a single atom sphere (instanced rendering).
struct SphereInstance {
    glm::vec3 position;
    float     radius;
    glm::vec4 color;
};

/// GPU data for a bond line / cylinder.
struct BondInstance {
    glm::vec3 posA, posB;
    float     thickness;
    glm::vec4 color;
};

/// GPU data for an electron cloud point.
struct CloudPoint {
    glm::vec3 position;
    glm::vec4 color;
};

// ─────────────────────────────────────────────────────────────
// Renderer — handles all OpenGL draw calls.
// ─────────────────────────────────────────────────────────────
class Renderer {
public:
    Renderer();
    ~Renderer();

    /// Must be called once after OpenGL context is ready.
    void init();

    /// Draw all atom nuclei as instanced spheres.
    void drawAtoms(const std::vector<SphereInstance>& atoms,
                   const glm::mat4& view, const glm::mat4& proj);

    /// Draw bonds between atoms as cylinders.
    void drawBonds(const std::vector<BondInstance>& bonds,
                   const glm::mat4& view, const glm::mat4& proj);

    /// Draw electron cloud as point sprites.
    void drawElectronCloud(const std::vector<CloudPoint>& points,
                           const glm::mat4& view, const glm::mat4& proj);

private:
    // Atom sphere mesh (unit sphere used with instancing)
    GLuint sphereVAO_ = 0, sphereVBO_ = 0, sphereEBO_ = 0;
    int    sphereIndexCount_ = 0;
    GLuint atomShader_ = 0;

    // Bond cylinder mesh
    GLuint cylinderVAO_ = 0, cylinderVBO_ = 0, cylinderEBO_ = 0;
    int    cylinderIndexCount_ = 0;
    GLuint bondShader_ = 0;

    // Electron cloud points
    GLuint cloudVAO_ = 0, cloudVBO_ = 0;
    GLuint cloudShader_ = 0;

    void buildSphereMesh(int stacks, int sectors);
    void buildCylinderMesh(int segments);
    void buildShaders();
};

} // namespace engine
