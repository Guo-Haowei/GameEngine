#include "path_tracer.h"

#include <algorithm>

#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/scene/scene.h"

namespace my {

gpu_bvh_t::gpu_bvh_t()
    : missIdx(-1), hitIdx(-1), leaf(0), geomIdx(-1) {
    _padding_0 = 0;
    _padding_1 = 0;
}

gpu_geometry_t::gpu_geometry_t()
    : kind(Kind::Invalid), material_id(-1) {}

gpu_geometry_t::gpu_geometry_t(const Vector3f& A, const Vector3f& B, const Vector3f& C, int material)
    : A(A), B(B), C(C), material_id(material) {
    kind = Kind::Triangle;
    CalcNormal();
}

gpu_geometry_t::gpu_geometry_t(const Vector3f& center, float radius, int material)
    : A(center), material_id(material) {
    // TODO: refactor
    kind = Kind::Sphere;
    this->radius = glm::max(0.01f, glm::abs(radius));
}

void gpu_geometry_t::CalcNormal() {
    using glm::cross;
    using glm::normalize;
    Vector3f BA = normalize(B - A);
    Vector3f CA = normalize(C - A);
    Vector3f norm = normalize(cross(BA, CA));
    normal1 = norm;
    normal2 = norm;
    normal3 = norm;
}

Vector3f gpu_geometry_t::Centroid() const {
    switch (kind) {
        case Kind::Triangle:
            return (A + B + C) / 3.0f;
        case Kind::Sphere:
            return A;
        default:
            assert(0);
            return Vector3f(0.0f);
    }
}

static Box3 Box3FromSphere(const gpu_geometry_t& sphere) {
    assert(sphere.kind == gpu_geometry_t::Kind::Sphere);

    return Box3(sphere.A - Vector3f(sphere.radius), sphere.A + Vector3f(sphere.radius));
}

static Box3 Box3FromTriangle(const gpu_geometry_t& triangle) {
    assert(triangle.kind == gpu_geometry_t::Kind::Triangle);

    Box3 ret = Box3(
        glm::min(triangle.C, glm::min(triangle.A, triangle.B)),
        glm::max(triangle.C, glm::max(triangle.A, triangle.B)));

    ret.MakeValid();
    return ret;
}

static Box3 Box3FromGeometry(const gpu_geometry_t& geom) {
    switch (geom.kind) {
        case gpu_geometry_t::Kind::Triangle:
            return Box3FromTriangle(geom);
        case gpu_geometry_t::Kind::Sphere:
            return Box3FromSphere(geom);
        default:
            assert(0);
    }
    return Box3();
}

static Box3 Box3FromGeometries(const GeometryList& geoms) {
    Box3 box;
    for (const gpu_geometry_t& geom : geoms) {
        box.UnionBox(Box3FromGeometry(geom));
    }

    box.MakeValid();
    return box;
}

#if 0
static Box3 Box3FromGeometriesCentroid(const GeometryList& geoms) {
    Box3 box;
    for (const gpu_geometry_t& geom : geoms) {
        box.ExpandPoint(geom.Centroid());
    }

    box.MakeValid();
    return box;
}
#endif

static int genIdx() {
    static int idx = 0;
    return idx++;
}

static int DominantAxis(const Box3& box) {
    const Vector3f span = box.Size();
    int axis = 0;
    if (span[axis] < span.y) {
        axis = 1;
    }
    if (span[axis] < span.z) {
        axis = 2;
    }

    return axis;
}

// @TODO: make it loop and also debug it visually
Bvh::Bvh(GeometryList& geometries, Bvh* parent)
    : m_idx(genIdx()), m_parent(parent) {
    m_left = nullptr;
    m_right = nullptr;
    m_leaf = false;
    m_hitIdx = -1;
    m_missIdx = -1;

    const size_t nGeoms = geometries.size();

    assert(nGeoms);

    if (nGeoms == 1) {
        m_geom = geometries.front();
        m_leaf = true;
        m_box = Box3FromGeometry(m_geom);
        return;
    }

    m_box = Box3FromGeometries(geometries);
    const float boxSurfaceArea = m_box.SurfaceArea();

    if (nGeoms <= 4 || boxSurfaceArea == 0.0f) {
        SplitByAxis(geometries);
        return;
    }

    constexpr int nBuckets = 12;
    struct BucketInfo {
        int count = 0;
        Box3 box;
    };
    BucketInfo buckets[nBuckets];

    std::vector<Vector3f> centroids(nGeoms);

    Box3 centroidBox;
    for (size_t i = 0; i < nGeoms; ++i) {
        centroids[i] = geometries.at(i).Centroid();
        centroidBox.ExpandPoint(centroids[i]);
    }

    const int axis = DominantAxis(centroidBox);
    const float tmin = centroidBox.GetMin()[axis];
    const float tmax = centroidBox.GetMax()[axis];

    for (size_t i = 0; i < nGeoms; ++i) {
        float tmp = ((centroids.at(i)[axis] - tmin) * nBuckets) / (tmax - tmin);
        int slot = static_cast<int>(tmp);
        slot = glm::clamp(slot, 0, nBuckets - 1);
        BucketInfo& bucket = buckets[slot];
        ++bucket.count;
        bucket.box.UnionBox(Box3FromGeometry(geometries.at(i)));
    }

    float costs[nBuckets - 1];
    for (int i = 0; i < nBuckets - 1; ++i) {
        Box3 b0, b1;
        int count0 = 0, count1 = 0;
        for (int j = 0; j <= i; ++j) {
            b0.UnionBox(buckets[j].box);
            count0 += buckets[j].count;
        }
        for (int j = i + 1; j < nBuckets; ++j) {
            b1.UnionBox(buckets[j].box);
            count1 += buckets[j].count;
        }

        constexpr float travCost = 0.125f;
        // constexpr float intersectCost = 1.f;
        costs[i] = travCost + (count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea()) / boxSurfaceArea;
    }

    int splitIndex = 0;
    float minCost = costs[splitIndex];
    for (int i = 0; i < nBuckets - 1; ++i) {
        // printf("cost of split after bucket %d is %f\n", i, costs[i]);
        if (costs[i] < minCost) {
            splitIndex = i;
            minCost = costs[splitIndex];
        }
    }

    // printf("split index is %d\n", splitIndex);

    GeometryList leftPartition;
    GeometryList rightPartition;

    for (const gpu_geometry_t& geom : geometries) {
        const Vector3f t = geom.Centroid();
        float tmp = (t[axis] - tmin) / (tmax - tmin);
        tmp *= nBuckets;
        int slot = static_cast<int>(tmp);
        slot = glm::clamp(slot, 0, nBuckets - 1);
        if (slot <= splitIndex) {
            leftPartition.push_back(geom);
        } else {
            rightPartition.push_back(geom);
        }
    }

    // printf("left has %llu\n", leftPartition.size());
    // printf("right has %llu\n", rightPartition.size());

    m_left = new Bvh(leftPartition, this);
    m_right = new Bvh(rightPartition, this);

    m_leaf = false;
}

Bvh::~Bvh() {
    // TODO: free memory
}

void Bvh::DiscoverIdx() {
    // hit link (find right link)
    m_missIdx = -1;
    for (const Bvh* cursor = m_parent; cursor; cursor = cursor->m_parent) {
        if (cursor->m_right && cursor->m_right->m_idx > m_idx) {
            m_missIdx = cursor->m_right->m_idx;
            break;
        }
    }

    m_hitIdx = m_left ? m_left->m_idx : m_missIdx;
}

void Bvh::CreateGpuBvh(GpuBvhList& outBvh, GeometryList& outGeometries) {
    DiscoverIdx();

    gpu_bvh_t gpuBvh;
    gpuBvh.min = m_box.GetMin();
    gpuBvh.max = m_box.GetMax();
    gpuBvh.hitIdx = m_hitIdx;
    gpuBvh.missIdx = m_missIdx;
    gpuBvh.leaf = m_leaf;
    gpuBvh.geomIdx = -1;
    if (m_leaf) {
        gpuBvh.geomIdx = static_cast<int>(outGeometries.size());
        outGeometries.push_back(m_geom);
    }

    outBvh.push_back(gpuBvh);
    if (m_left) {
        m_left->CreateGpuBvh(outBvh, outGeometries);
    }
    if (m_right) {
        m_right->CreateGpuBvh(outBvh, outGeometries);
    }
}

void Bvh::SplitByAxis(GeometryList& geoms) {
    class Sorter {
    public:
        bool operator()(const gpu_geometry_t& geom1, const gpu_geometry_t& geom2) {
            Box3 aabb1 = Box3FromGeometry(geom1);
            Box3 aabb2 = Box3FromGeometry(geom2);
            Vector3f center1 = aabb1.Center();
            Vector3f center2 = aabb2.Center();
            return center1[axis] < center2[axis];
        }

        Sorter(int axis)
            : axis(axis) {
            assert(axis < 3);
        }

    private:
        const unsigned int axis;
    };

    Sorter sorter(DominantAxis(m_box));

    std::sort(geoms.begin(), geoms.end(), sorter);
    const size_t mid = geoms.size() / 2;
    GeometryList leftPartition(geoms.begin(), geoms.begin() + mid);
    GeometryList rightPartition(geoms.begin() + mid, geoms.end());

    m_left = new Bvh(leftPartition, this);
    m_right = new Bvh(rightPartition, this);
    m_leaf = false;
}

void ConstructScene(const Scene& p_scene, GpuScene& p_out_scene) {
    /// materials
    using ecs::Entity;
    std::map<Entity, int> material_lut;

    [[maybe_unused]] const bool is_opengl = GraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL;

    p_out_scene.materials.clear();
    for (auto [entity, material] : p_scene.m_MaterialComponents) {
        gpu_material_t gpu_mat;
        gpu_mat.albedo = material.baseColor;
        gpu_mat.emissive = material.emissive * gpu_mat.albedo;
        gpu_mat.roughness = material.roughness;
        gpu_mat.reflect_chance = material.metallic;

        auto fill_texture = [&](int p_index, int& p_out_enabled, sampler2D& p_out_handle) {
            const Image* image = AssetRegistry::GetSingleton().GetAssetByHandle<Image>(material.textures[p_index].path);
            if (image && image->gpu_texture) {
                uint64_t handle = image->gpu_texture->GetResidentHandle();
                if (handle) {
                    p_out_enabled = true;
                    if (is_opengl) {
                        p_out_handle.Set64(handle);

                    } else {
                        p_out_handle.Set32(static_cast<int>(handle));
                    }
                    return true;
                }
            }
            return false;
        };

        fill_texture(MaterialComponent::TEXTURE_BASE, gpu_mat.has_base_color_map, gpu_mat.base_color_map_handle);
        fill_texture(MaterialComponent::TEXTURE_NORMAL, gpu_mat.has_normal_map, gpu_mat.normal_map_handle);
        fill_texture(MaterialComponent::TEXTURE_METALLIC_ROUGHNESS, gpu_mat.has_material_map, gpu_mat.material_map_handle);

        p_out_scene.materials.push_back(gpu_mat);
        material_lut[entity] = static_cast<int>(p_out_scene.materials.size()) - 1;
    }

    GeometryList tmp_gpu_objects;

    auto add_object = [&](const MeshComponent& p_mesh, const TransformComponent& p_transform) {
        // Loop over shapes
        Matrix4x4f transform = p_transform.GetWorldMatrix();

        for (size_t index = 0; index < p_mesh.indices.size(); index += 3) {
            const uint32_t index0 = p_mesh.indices[index];
            const uint32_t index1 = p_mesh.indices[index + 1];
            const uint32_t index2 = p_mesh.indices[index + 2];

            Vector3f points[3] = {
                transform * Vector4f(p_mesh.positions[index0], 1.0f),
                transform * Vector4f(p_mesh.positions[index1], 1.0f),
                transform * Vector4f(p_mesh.positions[index2], 1.0f),
            };

            Vector3f normals[3] = {
                transform * Vector4f(p_mesh.normals[index0], 0.0f),
                transform * Vector4f(p_mesh.normals[index1], 0.0f),
                transform * Vector4f(p_mesh.normals[index2], 0.0f),
            };

            Vector2f uvs[3] = {
                p_mesh.texcoords_0[index0],
                p_mesh.texcoords_0[index1],
                p_mesh.texcoords_0[index2],
            };

            // per-face material
            gpu_geometry_t triangle(points[0], points[1], points[2], 0);
            triangle.uv1 = uvs[0];
            triangle.uv2 = uvs[1];
            triangle.uv3 = uvs[2];
            triangle.normal1 = glm::normalize(normals[0]);
            triangle.normal2 = glm::normalize(normals[1]);
            triangle.normal3 = glm::normalize(normals[2]);
            triangle.kind = gpu_geometry_t::Kind::Triangle;
            auto it = material_lut.find(p_mesh.subsets[0].material_id);
            DEV_ASSERT(it != material_lut.end());
            triangle.material_id = it->second;
            tmp_gpu_objects.push_back(triangle);
        }
    };

    /// objects
    p_out_scene.geometries.clear();
    for (auto [entity, object] : p_scene.m_ObjectComponents) {
        // ObjectComponent
        auto transform = p_scene.GetComponent<TransformComponent>(entity);
        auto mesh = p_scene.GetComponent<MeshComponent>(object.meshId);
        DEV_ASSERT(transform);
        DEV_ASSERT(mesh);
        add_object(*mesh, *transform);
    }
    for (auto [entity, light] : p_scene.m_LightComponents) {
        if (light.GetType() == LIGHT_TYPE_INFINITE) {
            auto transform = p_scene.GetComponent<TransformComponent>(entity);
            DEV_ASSERT(transform);
            Vector3f rotation = glm::normalize(transform->GetWorldMatrix() * Vector4f(0.0, 0.0, 1.0f, 0.0f));
            float radius = 1000.0f;
            Vector3f center = radius * rotation * 5.0f;

            auto it = material_lut.find(entity);
            DEV_ASSERT(it != material_lut.end());
            int material_id = it->second;
            gpu_geometry_t sphere(center, radius, material_id);
            // HACK: increase emissive power
            p_out_scene.materials[material_id].emissive *= 5.0f;
            // gpu_geometry_t sphere(center, radius, it->second);
            tmp_gpu_objects.push_back(sphere);
        }
    }

    /// construct bvh
    Bvh root(tmp_gpu_objects);
    root.CreateGpuBvh(p_out_scene.bvhs, p_out_scene.geometries);

    p_out_scene.bbox = root.GetBox();

    /// adjust bbox
    for (gpu_bvh_t& bvh : p_out_scene.bvhs) {
        for (int i = 0; i < 3; ++i) {
            if (glm::abs(bvh.max[i] - bvh.min[i]) < 0.01f) {
                bvh.min[i] -= Box3::BOX_MIN_SIZE;
                bvh.max[i] += Box3::BOX_MIN_SIZE;
            }
        }
    }
}

}  // namespace my
