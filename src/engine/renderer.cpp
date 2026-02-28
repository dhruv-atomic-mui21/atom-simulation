#include "renderer.h"
#include "engine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace engine {

// ═════════════════════════════════════════════════════════════
//  Shader sources
// ═════════════════════════════════════════════════════════════

static const char* atomVertSrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vNormal;
out vec3 vFragPos;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vNormal  = normalize(mat3(transpose(inverse(uModel))) * aPos);
    gl_Position = uProj * uView * worldPos;
}
)";

static const char* atomFragSrc = R"(
#version 330 core
in vec3 vNormal;
in vec3 vFragPos;

uniform vec4  uColor;
uniform vec3  uLightDir;
uniform vec3  uViewPos;

out vec4 FragColor;

void main() {
    // Ambient
    vec3 ambient = 0.15 * uColor.rgb;
    // Diffuse
    float diff   = max(dot(normalize(vNormal), normalize(uLightDir)), 0.0);
    vec3  diffuse= diff * uColor.rgb;
    // Specular (Blinn-Phong)
    vec3  viewDir  = normalize(uViewPos - vFragPos);
    vec3  halfDir  = normalize(normalize(uLightDir) + viewDir);
    float spec     = pow(max(dot(normalize(vNormal), halfDir), 0.0), 64.0);
    vec3  specular = 0.4 * spec * vec3(1.0);

    FragColor = vec4(ambient + diffuse + specular, uColor.a);
}
)";

static const char* cloudVertSrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;

uniform mat4 uView;
uniform mat4 uProj;
uniform float uPointSize;

out vec4 vColor;

void main() {
    vColor = aColor;
    vec4 viewPos = uView * vec4(aPos, 1.0);
    gl_Position  = uProj * viewPos;
    // Scale point size by distance
    gl_PointSize = uPointSize / (-viewPos.z);
}
)";

static const char* cloudFragSrc = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    // Circular point sprite
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = dot(coord, coord);
    if (dist > 0.25) discard;
    float alpha = vColor.a * smoothstep(0.25, 0.1, dist);
    FragColor = vec4(vColor.rgb, alpha);
}
)";

// ═════════════════════════════════════════════════════════════
//  Renderer implementation
// ═════════════════════════════════════════════════════════════

Renderer::Renderer() = default;
Renderer::~Renderer() {
    if (sphereVAO_)   glDeleteVertexArrays(1, &sphereVAO_);
    if (sphereVBO_)   glDeleteBuffers(1, &sphereVBO_);
    if (sphereEBO_)   glDeleteBuffers(1, &sphereEBO_);
    if (cylinderVAO_) glDeleteVertexArrays(1, &cylinderVAO_);
    if (cylinderVBO_) glDeleteBuffers(1, &cylinderVBO_);
    if (cylinderEBO_) glDeleteBuffers(1, &cylinderEBO_);
    if (cloudVAO_)    glDeleteVertexArrays(1, &cloudVAO_);
    if (cloudVBO_)    glDeleteBuffers(1, &cloudVBO_);
    if (atomShader_)  glDeleteProgram(atomShader_);
    if (bondShader_)  glDeleteProgram(bondShader_);
    if (cloudShader_) glDeleteProgram(cloudShader_);
}

void Renderer::init() {
    buildShaders();
    buildSphereMesh(16, 24);
    buildCylinderMesh(12);

    // Cloud VAO — dynamic VBO, no data yet
    glGenVertexArrays(1, &cloudVAO_);
    glGenBuffers(1, &cloudVBO_);
    glBindVertexArray(cloudVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, cloudVBO_);
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CloudPoint),
                          (void*)offsetof(CloudPoint, position));
    glEnableVertexAttribArray(0);
    // color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(CloudPoint),
                          (void*)offsetof(CloudPoint, color));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Renderer::buildShaders() {
    atomShader_  = createShaderProgram(atomVertSrc,  atomFragSrc);
    bondShader_  = atomShader_; // reuse same Blinn-Phong shader for bonds
    cloudShader_ = createShaderProgram(cloudVertSrc, cloudFragSrc);
}

// ── Sphere mesh ──────────────────────────────────────────────
void Renderer::buildSphereMesh(int stacks, int sectors) {
    std::vector<float> verts;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i) {
        float theta = static_cast<float>(M_PI) * i / stacks;
        float sinT = std::sin(theta), cosT = std::cos(theta);
        for (int j = 0; j <= sectors; ++j) {
            float phi = 2.0f * static_cast<float>(M_PI) * j / sectors;
            float x = sinT * std::cos(phi);
            float y = cosT;
            float z = sinT * std::sin(phi);
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int cur  = i * (sectors + 1) + j;
            int next = cur + sectors + 1;
            indices.push_back(cur); indices.push_back(next); indices.push_back(cur + 1);
            indices.push_back(cur + 1); indices.push_back(next); indices.push_back(next + 1);
        }
    }
    sphereIndexCount_ = static_cast<int>(indices.size());

    glGenVertexArrays(1, &sphereVAO_);
    glGenBuffers(1, &sphereVBO_);
    glGenBuffers(1, &sphereEBO_);
    glBindVertexArray(sphereVAO_);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO_);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

// ── Cylinder mesh (unit cylinder along Y) ────────────────────
void Renderer::buildCylinderMesh(int segments) {
    std::vector<float> verts;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= 1; ++i) {
        float y = static_cast<float>(i);
        for (int j = 0; j <= segments; ++j) {
            float phi = 2.0f * static_cast<float>(M_PI) * j / segments;
            float x = std::cos(phi), z = std::sin(phi);
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
        }
    }
    for (int j = 0; j < segments; ++j) {
        int bot = j, top = j + segments + 1;
        indices.push_back(bot); indices.push_back(top);     indices.push_back(bot + 1);
        indices.push_back(bot + 1); indices.push_back(top); indices.push_back(top + 1);
    }
    cylinderIndexCount_ = static_cast<int>(indices.size());

    glGenVertexArrays(1, &cylinderVAO_);
    glGenBuffers(1, &cylinderVBO_);
    glGenBuffers(1, &cylinderEBO_);
    glBindVertexArray(cylinderVAO_);

    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO_);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cylinderEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

// ═════════════════════════════════════════════════════════════
//  Draw calls
// ═════════════════════════════════════════════════════════════

void Renderer::drawAtoms(const std::vector<SphereInstance>& atoms,
                         const glm::mat4& view, const glm::mat4& proj) {
    glUseProgram(atomShader_);
    glUniformMatrix4fv(glGetUniformLocation(atomShader_, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(atomShader_, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3f(glGetUniformLocation(atomShader_, "uLightDir"), 0.5f, 1.0f, 0.8f);

    // Extract camera position from view matrix
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 camPos(invView[3]);
    glUniform3fv(glGetUniformLocation(atomShader_, "uViewPos"), 1, glm::value_ptr(camPos));

    glBindVertexArray(sphereVAO_);
    for (const auto& a : atoms) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), a.position);
        model = glm::scale(model, glm::vec3(a.radius));
        glUniformMatrix4fv(glGetUniformLocation(atomShader_, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4fv(glGetUniformLocation(atomShader_, "uColor"), 1, glm::value_ptr(a.color));
        glDrawElements(GL_TRIANGLES, sphereIndexCount_, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}

void Renderer::drawBonds(const std::vector<BondInstance>& bonds,
                         const glm::mat4& view, const glm::mat4& proj) {
    glUseProgram(bondShader_);
    glUniformMatrix4fv(glGetUniformLocation(bondShader_, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(bondShader_, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3f(glGetUniformLocation(bondShader_, "uLightDir"), 0.5f, 1.0f, 0.8f);

    glm::mat4 invView = glm::inverse(view);
    glm::vec3 camPos(invView[3]);
    glUniform3fv(glGetUniformLocation(bondShader_, "uViewPos"), 1, glm::value_ptr(camPos));

    glBindVertexArray(cylinderVAO_);
    for (const auto& b : bonds) {
        glm::vec3 diff = b.posB - b.posA;
        float len = glm::length(diff);
        if (len < 1e-6f) continue;

        glm::vec3 dir = diff / len;
        // Build rotation from Y-axis to dir
        glm::vec3 up(0, 1, 0);
        glm::vec3 axis = glm::cross(up, dir);
        float axisLen = glm::length(axis);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), b.posA);
        if (axisLen > 1e-6f) {
            float angle = std::acos(glm::clamp(glm::dot(up, dir), -1.0f, 1.0f));
            model = glm::rotate(model, angle, glm::normalize(axis));
        } else if (dir.y < 0) {
            model = glm::rotate(model, static_cast<float>(M_PI), glm::vec3(1, 0, 0));
        }
        model = glm::scale(model, glm::vec3(b.thickness, len, b.thickness));

        glUniformMatrix4fv(glGetUniformLocation(bondShader_, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4fv(glGetUniformLocation(bondShader_, "uColor"), 1, glm::value_ptr(b.color));
        glDrawElements(GL_TRIANGLES, cylinderIndexCount_, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}

void Renderer::drawElectronCloud(const std::vector<CloudPoint>& points,
                                  const glm::mat4& view, const glm::mat4& proj) {
    if (points.empty()) return;

    glUseProgram(cloudShader_);
    glUniformMatrix4fv(glGetUniformLocation(cloudShader_, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(cloudShader_, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1f(glGetUniformLocation(cloudShader_, "uPointSize"), 80.0f);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glBindVertexArray(cloudVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, cloudVBO_);
    glBufferData(GL_ARRAY_BUFFER,
                 points.size() * sizeof(CloudPoint),
                 points.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_POINTS, 0, static_cast<int>(points.size()));
    glBindVertexArray(0);
    glDisable(GL_PROGRAM_POINT_SIZE);
}

} // namespace engine
