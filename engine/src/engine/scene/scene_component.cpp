#include "scene_component.h"

#include <yaml-cpp/yaml.h>

#include "engine/core/io/archive.h"
#include "engine/math/matrix_transform.h"
#include "engine/runtime/asset_registry.h"
#include "engine/runtime/graphics_manager_interface.h"

namespace my {

#pragma region TRANSFORM_COMPONENT
Matrix4x4f TransformComponent::GetLocalMatrix() const {
    Matrix4x4f rotationMatrix = glm::toMat4(Quaternion(m_rotation.w, m_rotation.x, m_rotation.y, m_rotation.z));
    Matrix4x4f translationMatrix = my::Translate(m_translation);
    Matrix4x4f scaleMatrix = my::Scale(m_scale);
    return translationMatrix * rotationMatrix * scaleMatrix;
}

bool TransformComponent::UpdateTransform() {
    if (IsDirty()) {
        SetDirty(false);
        m_worldMatrix = GetLocalMatrix();
        return true;
    }
    return false;
}

void TransformComponent::Scale(const Vector3f& p_scale) {
    SetDirty();
    m_scale.x *= p_scale.x;
    m_scale.y *= p_scale.y;
    m_scale.z *= p_scale.z;
}

void TransformComponent::Translate(const Vector3f& p_translation) {
    SetDirty();
    m_translation.x += p_translation.x;
    m_translation.y += p_translation.y;
    m_translation.z += p_translation.z;
}

void TransformComponent::Rotate(const Vector3f& p_euler) {
    SetDirty();
    glm::quat quaternion(m_rotation.w, m_rotation.x, m_rotation.y, m_rotation.z);
    glm::quat euler(glm::vec3(p_euler.x, p_euler.y, p_euler.z));
    quaternion = euler * quaternion;

    m_rotation.x = quaternion.x;
    m_rotation.y = quaternion.y;
    m_rotation.z = quaternion.z;
    m_rotation.w = quaternion.w;
}

void TransformComponent::SetLocalTransform(const Matrix4x4f& p_matrix) {
    SetDirty();
    Decompose(p_matrix, m_scale, m_rotation, m_translation);
}

void TransformComponent::MatrixTransform(const Matrix4x4f& p_matrix) {
    SetDirty();
    Decompose(p_matrix * GetLocalMatrix(), m_scale, m_rotation, m_translation);
}

void TransformComponent::UpdateTransformParented(const TransformComponent& p_parent) {
    CRASH_NOW();
    Matrix4x4f worldMatrix = GetLocalMatrix();
    const Matrix4x4f& worldMatrixParent = p_parent.m_worldMatrix;
    m_worldMatrix = worldMatrixParent * worldMatrix;
}
#pragma endregion TRANSFORM_COMPONENT

#pragma region MESH_COMPONENT
template<typename T>
static void InitVertexAttrib(MeshComponent::VertexAttribute& p_attrib, const std::vector<T>& p_buffer) {
    p_attrib.offsetInByte = 0;
    p_attrib.strideInByte = sizeof(p_buffer[0]);
    p_attrib.elementCount = static_cast<uint32_t>(p_buffer.size());
}

void MeshComponent::CreateRenderData() {
    // AABB
    localBound.MakeInvalid();
    for (MeshSubset& subset : subsets) {
        subset.local_bound.MakeInvalid();
        for (uint32_t i = 0; i < subset.index_count; ++i) {
            const Vector3f& point = positions[indices[i + subset.index_offset]];
            subset.local_bound.ExpandPoint(reinterpret_cast<const Vector3f&>(point));
        }
        subset.local_bound.MakeValid();
        localBound.UnionBox(subset.local_bound);
    }
    // Attributes
    for (int i = 0; i < std::to_underlying(VertexAttributeName::COUNT); ++i) {
        attributes[i].attribName = static_cast<VertexAttributeName>(i);
    }

    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::POSITION)], positions);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::NORMAL)], normals);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::TEXCOORD_0)], texcoords_0);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::TEXCOORD_1)], texcoords_1);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::TANGENT)], tangents);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::JOINTS_0)], joints_0);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::WEIGHTS_0)], weights_0);
    InitVertexAttrib(attributes[std::to_underlying(VertexAttributeName::COLOR_0)], color_0);
    return;
}
#pragma endregion MESH_COMPONENT

#pragma region MATERIAL_COMPONENT
#pragma endregion MATERIAL_COMPONENT

#pragma region CAMERA_COMPONENT
Matrix4x4f CameraComponent::CalcProjection() const {
    if (IsOrtho()) {
        const float half_height = m_orthoHeight * 0.5f;
        const float half_width = half_height * GetAspect();
        return BuildOrthoRH(-half_width,
                            half_width,
                            -half_height,
                            half_height,
                            m_near,
                            m_far);
    }
    return BuildPerspectiveRH(m_fovy.GetRadians(), GetAspect(), m_near, m_far);
}

Matrix4x4f CameraComponent::CalcProjectionGL() const {
    if (IsOrtho()) {
        const float half_height = m_orthoHeight * 0.5f;
        const float half_width = half_height * GetAspect();
        return BuildOpenGlOrthoRH(-half_width,
                                  half_width,
                                  -half_height,
                                  half_height,
                                  m_near,
                                  m_far);
    }
    return BuildOpenGlPerspectiveRH(m_fovy.GetRadians(), GetAspect(), m_near, m_far);
}

bool CameraComponent::Update() {
    if (IsDirty()) {
        SetDirty(false);

        m_front.x = m_yaw.Sin() * m_pitch.Cos();
        m_front.y = m_pitch.Sin();
        m_front.z = m_yaw.Cos() * -m_pitch.Cos();

        m_right = cross(m_front, Vector3f::UnitY);

        m_viewMatrix = LookAtRh(m_position, m_position + m_front, Vector3f::UnitY);

        // use gl matrix for frustum culling
        m_projectionMatrix = CalcProjectionGL();
        m_projectionViewMatrix = m_projectionMatrix * m_viewMatrix;
        return true;
    }

    return false;
}

void CameraComponent::SetDimension(int p_width, int p_height) {
    if (m_width != p_width || m_height != p_height) {
        m_width = p_width;
        m_height = p_height;
        SetDirty();
    }
}

void CameraComponent::SetOrtho(bool p_flag) {
    const bool has_flag = flags & IS_ORTHO;
    if (has_flag != p_flag) {
        p_flag ? flags |= IS_ORTHO : flags &= ~IS_ORTHO;
        SetDirty();
    }
}
#pragma endregion CAMERA_COMPONENT

#pragma region LUA_SCRIPT_COMPONENT
LuaScriptComponent& LuaScriptComponent::SetClassName(std::string_view p_class_name) {
    if (DEV_VERIFY(!p_class_name.empty())) {
        m_className = p_class_name;
    }

    return *this;
}

LuaScriptComponent& LuaScriptComponent::SetPath(std::string_view p_path) {
    if (p_path != m_path) {
        // LOG_VERBOSE("changing script '{}' to '{}'", m_path, p_path);
        m_path = p_path;
    }

    return *this;
}
#pragma endregion LUA_SCRIPT_COMPONENT

#pragma region NATIVE_SCRIPT_COMPONENT
NativeScriptComponent::~NativeScriptComponent() {
    // DON'T DELETE instance, because it could be copied from other script
    // Memory leak!!!
}

NativeScriptComponent::NativeScriptComponent(const NativeScriptComponent& p_rhs) {
    *this = p_rhs;
}

NativeScriptComponent& NativeScriptComponent::operator=(const NativeScriptComponent& p_rhs) {
    instantiateFunc = p_rhs.instantiateFunc;
    destroyFunc = p_rhs.destroyFunc;
    instance = p_rhs.instance;
    return *this;
}
#pragma endregion NATIVE_SCRIPT_COMPONENT

#pragma region RIGID_BODY_COMPONENT
RigidBodyComponent& RigidBodyComponent::InitCube(const Vector3f& p_half_size) {
    shape = SHAPE_CUBE;
    size = p_half_size;
    return *this;
}

RigidBodyComponent& RigidBodyComponent::InitSphere(float p_radius) {
    shape = SHAPE_SPHERE;
    size = Vector3f(p_radius);
    return *this;
}

RigidBodyComponent& RigidBodyComponent::InitGhost() {
    objectType = GHOST;
    mass = 1.0f;
    return *this;
}
#pragma endregion RIGID_BODY_COMPONENT

#pragma region MESH_EMITTER_COMPONENT
void MeshEmitterComponent::Reset() {
    if ((int)particles.size() != maxMeshCount) {
        particles.resize(maxMeshCount);
    }

    aliveList.clear();
    aliveList.reserve(maxMeshCount);
    deadList.clear();
    deadList.reserve(maxMeshCount);
    for (int i = 0; i < maxMeshCount; ++i) {
        deadList.emplace_back(i);
    }
}

void MeshEmitterComponent::UpdateParticle(Index p_index, float p_timestep) {
    DEV_ASSERT(p_index.v < particles.size());
    auto& p = particles[p_index.v];
    DEV_ASSERT(p.lifespan >= 0.0f);

    p.scale *= (1.0f - p_timestep);
    p.scale = max(p.scale, 0.1f);
    p.velocity += p_timestep * gravity;
    p.rotation += Vector3f(p_timestep);
    p.lifespan -= p_timestep;

    p.position += p_timestep * p.velocity;
}

#pragma endregion MESH_EMITTER_COMPONENT

#pragma region SOFT_BODY_COMPONENT
#pragma endregion SOFT_BODY_COMPONENT

#pragma region ENVIRONMENT_COMPONENT
#pragma endregion ENVIRONMENT_COMPONENT

#pragma region TILE_MAP_COMPONENT
void TileMapComponent::FromArray(const std::vector<std::vector<int>>& p_data) {
    m_width = static_cast<int>(p_data[0].size());
    m_height = static_cast<int>(p_data.size());

    m_tiles.resize(m_width * m_height);
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            m_tiles[y * m_width + x] = p_data[y][x];
        }
    }

    SetDirty();
}

void TileMapComponent::SetTile(int p_x, int p_y, int p_tile_id) {
    if (p_x >= 0 && p_x < m_width && p_y >= 0 && p_y < m_height) {
        int& old = m_tiles[p_y * m_width + p_x];
        if (old != p_tile_id) {
            old = p_tile_id;
            SetDirty();
        }
    }
}

int TileMapComponent::GetTile(int p_x, int p_y) const {
    if (p_x >= 0 && p_x < m_width && p_y >= 0 && p_y < m_height) {
        return m_tiles[p_y * m_width + p_x];
    }
    return 0;
}

void TileMapComponent::SetDimensions(int p_width, int p_height) {
    m_width = p_width;
    m_height = p_height;
    m_tiles.resize(p_width * p_height);
}

void TileMapComponent::CreateRenderData() {
    const int w = m_width;
    const int h = m_height;
    DEV_ASSERT(w && h);

    std::vector<Vector2f> vertices;
    std::vector<Vector2f> uvs;
    std::vector<uint32_t> indices;
    vertices.reserve(w * h * 4);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int tile = m_tiles[y * m_width + x];
            if (tile == 0) {
                continue;
            }

            const float s = 1.0f;
            float x0 = s * x;
            float y0 = s * y;
            float x1 = s * (x + 1);
            float y1 = s * (y + 1);
            Vector2f bottom_left{ x0, y0 };
            Vector2f bottom_right{ x1, y0 };
            Vector2f top_left{ x0, y1 };
            Vector2f top_right{ x1, y1 };
            Vector2f uv0{ 0, 0 };
            Vector2f uv1{ 1, 0 };
            Vector2f uv2{ 0, 1 };
            Vector2f uv3{ 1, 1 };

            const uint32_t offset = (uint32_t)vertices.size();
            vertices.push_back(bottom_left);
            vertices.push_back(bottom_right);
            vertices.push_back(top_left);
            vertices.push_back(top_right);

            uvs.push_back(uv0);
            uvs.push_back(uv1);
            uvs.push_back(uv2);
            uvs.push_back(uv3);

            indices.push_back(0 + offset);
            indices.push_back(1 + offset);
            indices.push_back(3 + offset);

            indices.push_back(0 + offset);
            indices.push_back(3 + offset);
            indices.push_back(2 + offset);
        }
    }
    uint32_t count = (uint32_t)indices.size();

    GpuBufferDesc buffers[2];
    GpuBufferDesc buffer_desc;
    buffer_desc.type = GpuBufferType::VERTEX;
    buffer_desc.elementSize = sizeof(Vector2f);
    buffer_desc.elementCount = (uint32_t)vertices.size();
    buffer_desc.initialData = vertices.data();

    buffers[0] = buffer_desc;

    buffer_desc.initialData = uvs.data();
    buffers[1] = buffer_desc;

    GpuBufferDesc index_desc;
    index_desc.type = GpuBufferType::INDEX;
    index_desc.elementSize = sizeof(uint32_t);
    index_desc.elementCount = count;
    index_desc.initialData = indices.data();

    GpuMeshDesc desc;
    desc.drawCount = count;
    desc.enabledVertexCount = 2;
    desc.vertexLayout[0] = GpuMeshDesc::VertexLayout{ 0, sizeof(Vector2f), 0 };
    desc.vertexLayout[1] = GpuMeshDesc::VertexLayout{ 1, sizeof(Vector2f), 0 };

    auto mesh = IGraphicsManager::GetSingleton().CreateMeshImpl(desc, 2, buffers, &index_desc);

    m_mesh = *mesh;
}

#pragma endregion TILE_MAP_COMPONENT

}  // namespace my
