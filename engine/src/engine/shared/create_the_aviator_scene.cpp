#include "engine/core/base/random.h"
#include "engine/core/framework/display_manager.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/math/color.h"
#include "engine/core/math/geometry.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/scene/scene.h"
#include "engine/scene/scriptable_entity.h"

namespace std {

template<>
struct hash<my::Vector3f> {
    std::size_t operator()(const my::Vector3f& p_vec) const {
        std::hash<uint8_t> byte_hash;
        size_t result = 0;

        const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(&p_vec);

        // Hash each byte of the memory block
        for (size_t i = 0; i < sizeof(p_vec); ++i) {
            result ^= byte_hash(byte_ptr[i]) + 0x9e3779b9 + (result << 6) + (result >> 2);
        }

        return result;
    }
};

}  // namespace std

namespace my {

static constexpr float OCEAN_RADIUS = 240.0f;
static constexpr int CLOUD_COUNT = 20;
static constexpr float WORLD_SPEED = 0.3f;
static constexpr float ENTITY_LIFE_TIME = 1.5f * glm::pi<float>() / WORLD_SPEED - 3.0f;
static constexpr float MIN_HEIGHT = 15.f;
static constexpr float MAX_HEIGHT = 45.f;
static constexpr float AMP_WIDTH = 30.0f;
static constexpr float AMP_HEIGHT = 32.0f;
static constexpr float OBSTACLE_RADIUS = 4.0f;

static const Color RED_COLOR = Color::Hex(0xCE190A);
static const Color WHITE_COLOR = Color::Hex(0XD8D0D1);
static const Color BROWN_COLOR = Color::Hex(0x59332E);
static const Color DRAK_BROWN_COLOR = Color::Hex(0x23190F);
static const Color BLUE_COLOR = Color::Hex(0X10A8A3);
// static const Color PINK_COLOR = Color::Hex(0xF5986E);

// @TODO:
// * cascaded shadow map
// * alpha blending
// * fog
// * 2D UI

static float Normalize(float p_value, float p_low, float p_high, float p_clamp_low, float p_clamp_high) {
    const float bounded_value = glm::clamp(p_value, p_low, p_high);
    return (bounded_value - p_low) / (p_high - p_low) * (p_clamp_high - p_clamp_low) + p_clamp_low;
}

static MeshComponent MakeOceanMesh(float p_radius,
                                   float p_height,
                                   int p_sectors,
                                   int p_height_sector) {
    MeshComponent mesh;

    auto& indices = mesh.indices;
    constexpr float pi = glm::pi<float>();

    std::array<float, 2> heights = { 0.5f * p_height, -0.5f * p_height };

    // cylinder side
    float height_step = (float)p_height / p_height_sector;
    for (float y = heights[1]; y < heights[0]; y += height_step) {
        for (int index = 0; index < p_sectors; ++index) {
            uint32_t point_offset = (uint32_t)mesh.positions.size();

            float angle_1 = 2.0f * pi * index / p_sectors;
            float x_1 = p_radius * glm::cos(angle_1);
            float z_1 = p_radius * glm::sin(angle_1);
            float angle_2 = 2.0f * pi * ((index + 1) == p_sectors ? 0 : (index + 1)) / p_sectors;
            float x_2 = p_radius * glm::cos(angle_2);
            float z_2 = p_radius * glm::sin(angle_2);

            Vector3f point_1(x_1, y, z_1);
            Vector3f point_2(x_1, y + height_step, z_1);

            Vector3f point_3(x_2, y, z_2);
            Vector3f point_4(x_2, y + height_step, z_2);

            Vector3f AB = point_1 - point_2;
            Vector3f AC = point_1 - point_3;
            Vector3f normal = glm::normalize(glm::cross(AB, AC));

            mesh.positions.emplace_back(point_1);
            mesh.positions.emplace_back(point_2);
            mesh.positions.emplace_back(point_3);

            mesh.positions.emplace_back(point_2);
            mesh.positions.emplace_back(point_4);
            mesh.positions.emplace_back(point_3);

            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);

            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());

            const uint32_t a = point_offset + 0;
            const uint32_t b = point_offset + 1;
            const uint32_t c = point_offset + 2;
            const uint32_t d = point_offset + 3;
            const uint32_t e = point_offset + 4;
            const uint32_t f = point_offset + 5;

            indices.emplace_back(a);
            indices.emplace_back(b);
            indices.emplace_back(c);
            indices.emplace_back(d);
            indices.emplace_back(e);
            indices.emplace_back(f);
        }
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

Scene* CreateTheAviatorScene() {
    const std::array<Vector3f, 8> cockpit_points = {
        Vector3f(-4.f, +1.5f, +0.5f),  // A
        Vector3f(-4.f, +0.5f, +0.5f),  // B
        Vector3f(+4.f, -2.5f, +2.5f),  // C
        Vector3f(+4.f, +2.5f, +2.5f),  // D
        Vector3f(-4.f, +1.5f, -0.5f),  // E
        Vector3f(-4.f, +0.5f, -0.5f),  // F
        Vector3f(+4.f, -2.5f, -2.5f),  // G
        Vector3f(+4.f, +2.5f, -2.5f),  // H
    };

    const float plane_height = 30.0f;

    ecs::Entity::SetSeed();

    Scene* scene = new Scene;
    scene->m_physicsMode = PhysicsMode::COLLISION_DETECTION;

    auto root = scene->CreateTransformEntity("root");
    scene->m_root = root;

    Vector2i frame_size = DVAR_GET_IVEC2(resolution);
    // editor camera
    {
        auto editor_camera = scene->CreatePerspectiveCameraEntity("editor_camera", frame_size.x, frame_size.y);
        auto camera = scene->GetComponent<PerspectiveCameraComponent>(editor_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0.0f, plane_height + 10.0f, 80.0f));
        camera->SetEditorCamera();
        scene->AttachChild(editor_camera, root);
    }
    // main camera
    {
        auto main_camera = scene->CreatePerspectiveCameraEntity("main_camera", frame_size.x, frame_size.y);
        scene->AttachChild(main_camera, root);

        auto camera = scene->GetComponent<PerspectiveCameraComponent>(main_camera);
        DEV_ASSERT(camera);
        camera->SetPosition(Vector3f(0.0f, plane_height + 10.0f, 80.0f));
        camera->SetPrimary();

        class CameraController : public ScriptableEntity {
        protected:
            void OnCollision(ecs::Entity p_other_id) override {
                unused(p_other_id);
                __debugbreak();
            }

            void OnUpdate(float p_timestep) override {
                PerspectiveCameraComponent* camera = GetComponent<PerspectiveCameraComponent>();
                if (camera) {
                    const Vector2f mouse_move = InputManager::GetSingleton().MouseMove();
                    if (glm::abs(mouse_move.x) > 2.0f) {
                        float angle = camera->GetFovy().GetDegree();
                        angle += mouse_move.x * p_timestep * 2.0f;
                        angle = glm::clamp(angle, 35.0f, 80.0f);
                        camera->SetFovy(Degree(angle));
                    }
                }
            }
        };

        scene->Create<NativeScriptComponent>(main_camera).Bind<CameraController>();
    }

    // create light
    auto light = scene->CreateInfiniteLightEntity("sun");
    {
        scene->AttachChild(light, root);
        TransformComponent* transform = scene->GetComponent<TransformComponent>(light);
        transform->RotateX(Degree(-41.f));
        transform->RotateY(Degree(-1.f));
        transform->RotateZ(Degree(-30.f));
        LightComponent* light_component = scene->GetComponent<LightComponent>(light);
        light_component->SetCastShadow();
    }

#pragma region SETUP_MATERIALS

    constexpr float default_roughness = 0.8f;
    constexpr float default_metallic = 0.2f;
    ecs::Entity material_red = scene->CreateMaterialEntity("material_red");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_red);
        material->baseColor = RED_COLOR.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_white = scene->CreateMaterialEntity("material_white");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_white);
        material->baseColor = WHITE_COLOR.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_dark_brown = scene->CreateMaterialEntity("material_dark_brown");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_dark_brown);
        material->baseColor = DRAK_BROWN_COLOR.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_brown = scene->CreateMaterialEntity("material_brown");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_brown);
        material->baseColor = BROWN_COLOR.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
    ecs::Entity material_blue = scene->CreateMaterialEntity("material_blue");
    {
        MaterialComponent* material = scene->GetComponent<MaterialComponent>(material_blue);
        material->baseColor = BLUE_COLOR.ToVector4f();
        material->roughness = default_roughness;
        material->metallic = default_metallic;
    }
#pragma endregion SETUP_MATERIALS

#pragma region SETUP_PLANE
    // create plane
    auto plane = scene->CreateTransformEntity("plane");
    {
        TransformComponent* transform = scene->GetComponent<TransformComponent>(plane);
        transform->Translate(Vector3f(0.0f, plane_height, 0.0f));

        auto& rigid_body = scene->Create<RigidBodyComponent>(plane)
                               .InitSphere(4.0f);
        rigid_body.mass = 1.0f;

        scene->AttachChild(plane, root);

        class PlaneScript : public ScriptableEntity {
            void OnCollision(ecs::Entity p_other_id) override {
                NameComponent* name = m_scene->GetComponent<NameComponent>(p_other_id);
                LOG_ERROR("collide with {}", name->GetName());
            }

            void OnCreate() override {
            }

            void OnUpdate(float p_timestep) override {
                if (InputManager::GetSingleton().IsButtonDown(MouseButton::RIGHT)) {
                    __debugbreak();
                }

                const auto [width, height] = DisplayManager::GetSingleton().GetWindowSize();
                Vector2f mouse = InputManager::GetSingleton().GetCursor();
                mouse.x /= (float)width;
                mouse.y /= (float)height;
                mouse = 2.0f * mouse - 1.0f;
                mouse.y = -mouse.y;

                TransformComponent* transform = GetComponent<TransformComponent>();

                Vector3f translate = transform->GetTranslation();

                const float target_x = Normalize(mouse.x, -1.0f, 1.0f, -AMP_WIDTH, -0.7f * AMP_WIDTH);
                const float target_y = Normalize(mouse.y, -0.75f, 0.75f, translate.y - AMP_HEIGHT, translate.y + AMP_HEIGHT);

                const float speed = 3.0f;
                Vector2f delta(target_x - translate.x, target_y - translate.y);
                delta *= p_timestep * speed;

                translate.x += delta.x;
                translate.y += delta.y;
                translate.y = glm::clamp(translate.y, MIN_HEIGHT, MAX_HEIGHT);

                transform->SetTranslation(translate);

                float rotate_z_angle = 0.3f * delta.y;
                rotate_z_angle = glm::clamp(rotate_z_angle, glm::radians(-60.0f), glm::radians(60.0f));
                Quaternion q(Vector3f(0.0f, 0.0f, rotate_z_angle));
                transform->SetRotation(Vector4f(q.x, q.y, q.z, q.w));
            }
        };

        scene->Create<NativeScriptComponent>(plane).Bind<PlaneScript>();
    }
    {
        auto cockpit = scene->CreateMeshEntity("cockpit",
                                               material_red,
                                               MakeCubeMesh(cockpit_points));
        scene->AttachChild(cockpit, plane);
    }
    {
        auto engine = scene->CreateCubeEntity("engine",
                                              material_white,
                                              Vector3f(1.0f, 2.5f, 2.5f),
                                              glm::translate(Vector3f(5, 0, 0)));
        scene->AttachChild(engine, plane);
    }
    {
        auto side_wing = scene->CreateCubeEntity("side_wing",
                                                 material_red,
                                                 Vector3f(1.5f, 0.25f, 6.0f),
                                                 glm::translate(Vector3f(0.0f, 1.5f, 0.0f)));
        scene->AttachChild(side_wing, plane);
    }
    {
        auto tail = scene->CreateCubeEntity("tail",
                                            material_red,
                                            Vector3f(0.75f, 1.0f, 0.25f),
                                            glm::translate(Vector3f(-4, 2, 0)));
        scene->AttachChild(tail, plane);
    }
    {
        auto wind_shield = scene->CreateCubeEntity("wind_shield",
                                                   material_white,
                                                   Vector3f(0.15f, 0.75f, 1.0f),
                                                   glm::translate(Vector3f(0.5f, 2.7f, 0.0f)));
        scene->AttachChild(wind_shield, plane);
    }
    {
        auto wheel_protection_1 = scene->CreateCubeEntity("wheel_protection_1",
                                                          material_red,
                                                          Vector3f(1.5f, 0.75f, 0.5f),
                                                          glm::translate(Vector3f(2.5f, -2.0f, 2.5f)));
        scene->AttachChild(wheel_protection_1, plane);
    }
    {
        auto wheel_protection_2 = scene->CreateCubeEntity("wheel_protection_2",
                                                          material_red,
                                                          Vector3f(1.5f, 0.75f, 0.5f),
                                                          glm::translate(Vector3f(2.5f, -2.0f, -2.5f)));
        scene->AttachChild(wheel_protection_2, plane);
    }
    {
        auto tire_1 = scene->CreateCubeEntity("tire_1",
                                              material_dark_brown,
                                              Vector3f(1.2f, 1.2f, 0.2f),
                                              glm::translate(Vector3f(2.5f, -2.8f, 2.5f)));
        scene->AttachChild(tire_1, plane);
    }
    {
        auto tire_2 = scene->CreateCubeEntity("tire_2",
                                              material_dark_brown,
                                              Vector3f(1.2f, 1.2f, 0.2f),
                                              glm::translate(Vector3f(2.5f, -2.8f, -2.5f)));
        scene->AttachChild(tire_2, plane);
    }
    {
        auto wheel_axis = scene->CreateCubeEntity("wheel_axis",
                                                  material_brown,
                                                  Vector3f(0.5f, 0.5f, 2.95f),
                                                  glm::translate(Vector3f(2.5f, -2.8f, -0.0f)));
        scene->AttachChild(wheel_axis, plane);
    }

    {
        Matrix4x4f rotation = glm::rotate(Degree(160.f).GetRadians(), Vector3f(0, 0, 1));
        Matrix4x4f translation = glm::translate(Vector3f(-3.3f, -0.2f, 0.0f));
        auto suspension = scene->CreateCubeEntity("suspension",
                                                  material_red,
                                                  Vector3f(0.2f, 1.0f, 0.2f),
                                                  translation * rotation);
        scene->AttachChild(suspension, plane);
    }
    {
        auto tire_3 = scene->CreateCubeEntity("tire_3",
                                              material_brown,
                                              Vector3f(0.4f, 0.4f, 0.15f),
                                              glm::translate(Vector3f(-3.5f, -0.8f, 0.0f)));
        scene->AttachChild(tire_3, plane);
    }
    {
        auto tire_4 = scene->CreateCubeEntity("tire_4",
                                              material_dark_brown,
                                              Vector3f(0.6f, 0.6f, 0.1f),
                                              glm::translate(Vector3f(-3.5f, -0.8f, 0.0f)));
        scene->AttachChild(tire_4, plane);
    }

    auto propeller = scene->CreateTransformEntity("propeller");
    {
        TransformComponent* transform = scene->GetComponent<TransformComponent>(propeller);
        transform->Translate(Vector3f(6.0f, 0.0f, 0.0f));
        scene->AttachChild(propeller, plane);
        auto& script = scene->Create<LuaScriptComponent>(propeller);
        script.SetScript("@res://scripts/propeller.lua");
    }
    {
        auto pivot = scene->CreateMeshEntity("pivot",
                                             material_brown,
                                             MakeConeMesh(0.4f, 2.0f, 8));
        TransformComponent* transform = scene->GetComponent<TransformComponent>(pivot);
        transform->RotateZ(Degree(90.0f));
        scene->AttachChild(pivot, propeller);
    }
    {
        auto blade1 = scene->CreateCubeEntity("blade1",
                                              material_dark_brown,
                                              Vector3f(0.2f, 4.0f, 0.2f),
                                              glm::translate(Vector3f(0.4f, 0.0f, 0.0f)));
        scene->AttachChild(blade1, propeller);
    }
    {
        auto blade2 = scene->CreateCubeEntity("blade2",
                                              material_dark_brown,
                                              Vector3f(0.2f, 0.2f, 4.0f),
                                              glm::translate(Vector3f(0.4f, 0.0f, 0.0f)));
        scene->AttachChild(blade2, propeller);
    }
#pragma endregion SETUP_PLANE

    auto world = scene->CreateTransformEntity("world");
    {
        scene->AttachChild(world, root);
        auto transform = scene->GetComponent<TransformComponent>(world);
        transform->Translate(Vector3f(0.0f, -OCEAN_RADIUS, 0.0f));
    }

    auto earth = scene->CreateTransformEntity("earth");
    {
        scene->AttachChild(earth, world);

        class EarthScript : public ScriptableEntity {
            void OnCollision(ecs::Entity p_other_id) override {
                unused(p_other_id);
                __debugbreak();
            }

            void OnUpdate(float p_timestep) override {
                auto transform = GetComponent<TransformComponent>();
                transform->Rotate(Vector3f(0.0f, 0.0f, p_timestep * WORLD_SPEED));
            }
        };
        scene->Create<NativeScriptComponent>(earth).Bind<EarthScript>();
    }

    // generator
    {
        auto generator = scene->CreateTransformEntity("generator");
        scene->AttachChild(generator, earth);

        class GeneratorScript : public ScriptableEntity {
        protected:
            void OnCollision(ecs::Entity p_other_id) override {
                unused(p_other_id);
                __debugbreak();
            }

            void OnCreate() override {
                CreateObstacleResource();

                m_obstaclePool.resize(OBSTACLE_POOL_SIZE);
                int counter = 0;
                for (auto& obstacle : m_obstaclePool) {
                    obstacle.lifeRemains = 0.0f;
                    obstacle.id = m_scene->CreateObjectEntity(std::format("battery_{}", ++counter));
                    ObjectComponent* object = m_scene->GetComponent<ObjectComponent>(obstacle.id);
                    object->meshId = m_obstacleMesh;

                    m_scene->AttachChild(obstacle.id, m_id);
                    m_obstacleDeadList.push_back(&obstacle);

                    auto& rigid_body = m_scene->Create<RigidBodyComponent>(obstacle.id)
                                           .InitSphere(0.5f * OBSTACLE_RADIUS);

                    rigid_body.mass = 1.0f;
                }
            }

            void OnUpdate(float p_timestep) override {
                m_time += p_timestep;

                // spawn object
                if (m_time - m_lastSpawnTime > 1.0f) {
                    if (!m_obstacleDeadList.empty()) {
                        auto* obstacle = m_obstacleDeadList.front();
                        m_obstacleDeadList.pop_front();
                        obstacle->lifeRemains = ENTITY_LIFE_TIME;

                        TransformComponent* transform = m_scene->GetComponent<TransformComponent>(obstacle->id);
                        float angle = m_time * WORLD_SPEED;
                        angle += Degree(90.0f).GetRadians();
                        const float distance = OCEAN_RADIUS + Random::Float(MIN_HEIGHT, MAX_HEIGHT);
                        transform->SetTranslation(distance * Vector3f(glm::sin(angle), glm::cos(angle), 0.0f));
                        m_lastSpawnTime = m_time;
                        m_obstacleAliveList.emplace_back(obstacle);
                    }
                }

                std::list<Obstacle*> tmp;
                for (int i = (int)m_obstacleAliveList.size() - 1; i >= 0; --i) {
                    Obstacle* obstacle = m_obstacleAliveList.back();
                    m_obstacleAliveList.pop_back();
                    obstacle->lifeRemains -= p_timestep;
                    if (obstacle->lifeRemains <= 0.0f) {
                        TransformComponent* transform = m_scene->GetComponent<TransformComponent>(obstacle->id);
                        // HACK: move the dead component away
                        transform->Translate(Vector3f(0.0f, -1000.0f, 0.0f));
                        m_obstacleDeadList.emplace_back(obstacle);
                    } else {
                        tmp.emplace_back(obstacle);
                    }
                }
                m_obstacleAliveList = std::move(tmp);
            }

            void CreateObstacleResource() {
                m_obstacleMesh = m_scene->CreateMeshEntity("obstacle_mesh");
                MeshComponent* mesh = m_scene->GetComponent<MeshComponent>(m_obstacleMesh);
                *mesh = MakeSphereMesh(OBSTACLE_RADIUS, 6, 6);
                mesh->gpuResource = GraphicsManager::GetSingleton().CreateMesh(*mesh);
                DEV_ASSERT(!mesh->subsets.empty());
                m_obstacleMaterial = m_scene->CreateMaterialEntity("obstacle_material");
                MaterialComponent* material = m_scene->GetComponent<MaterialComponent>(m_obstacleMaterial);
                material->baseColor = Vector4f(RED_COLOR.r, RED_COLOR.g, RED_COLOR.b, 1.0f);
                mesh->subsets[0].material_id = m_obstacleMaterial;
            }

        private:
            ecs::Entity m_obstacleMesh;
            ecs::Entity m_obstacleMaterial;
            ecs::Entity m_tetrahedronMesh;
            float m_time{ 0.0f };
            float m_lastSpawnTime{ 0.0f };

            struct Obstacle {
                ecs::Entity id;
                float lifeRemains;
            };

            enum : uint32_t {
                OBSTACLE_POOL_SIZE = 4,
                // OBSTACLE_POOL_SIZE = 32,
            };
            std::vector<Obstacle> m_obstaclePool;
            std::list<Obstacle*> m_obstacleDeadList;
            std::list<Obstacle*> m_obstacleAliveList;
        };

        scene->Create<NativeScriptComponent>(generator).Bind<GeneratorScript>();
    }

#pragma region SETUP_OCEAN
    // ocean
    {
        auto ocean = scene->CreateMeshEntity("ocean", material_blue, MakeOceanMesh(OCEAN_RADIUS, 320.0f, 60, 16));
        ObjectComponent* object = scene->GetComponent<ObjectComponent>(ocean);
        DEV_ASSERT(object);

        MeshComponent* mesh = scene->GetComponent<MeshComponent>(object->meshId);
        DEV_ASSERT(mesh);
        mesh->flags |= MeshComponent::DYNAMIC;

        auto transform = scene->GetComponent<TransformComponent>(ocean);
        DEV_ASSERT(transform);
        transform->RotateX(Degree(90.0f));
        transform->RotateZ(Degree(90.0f));
        scene->AttachChild(ocean, earth);

        class OceanScript : public ScriptableEntity {
            struct Wave {
                float angle;
                float amp;
                float speed;
            };

            void OnCollision(ecs::Entity p_other_id) override {
                unused(p_other_id);
                __debugbreak();
            }

            void OnCreate() override {
                const ObjectComponent* object = GetComponent<ObjectComponent>();
                DEV_ASSERT(object);
                const MeshComponent* mesh = m_scene->GetComponent<MeshComponent>(object->meshId);
                DEV_ASSERT(mesh);

                std::unordered_map<Vector3f, Wave> cache;

                for (const Vector3f& position : mesh->positions) {

                    auto it = cache.find(position);
                    if (it != cache.end()) {
                        m_waves.push_back(it->second);
                        continue;
                    }

                    Wave wave;
                    wave.angle = Random::Float() * 2.0f * glm::pi<float>();
                    wave.amp = 3.5f + Random::Float() * 3.5f;
                    wave.speed = 6.f * (Random::Float() * 1.0f);
                    m_waves.emplace_back(wave);
                    cache[position] = wave;
                }
            }

            void OnUpdate(float p_timestep) override {
                const ObjectComponent* object = GetComponent<ObjectComponent>();
                DEV_ASSERT(object);
                const MeshComponent* mesh = m_scene->GetComponent<MeshComponent>(object->meshId);
                DEV_ASSERT(mesh);

                auto& positions = mesh->updatePositions;
                auto& normals = mesh->updateNormals;
                positions.clear();
                normals.clear();
                positions.reserve(mesh->positions.size());
                normals.reserve(mesh->normals.size());

                for (size_t idx = 0; idx < mesh->positions.size(); idx += 3) {
                    Vector3f points[3];
                    for (int i = 0; i < 3; ++i) {
                        Wave& wave = m_waves[idx + i];
                        const Vector3f& position = mesh->positions[idx + i];
                        Vector3f new_position = position;
                        new_position.x += glm::cos(wave.angle) * wave.amp;
                        new_position.z += glm::sin(wave.angle) * wave.amp;
                        positions.emplace_back(new_position);
                        wave.angle += p_timestep * wave.speed;

                        points[i] = new_position;
                    }

                    Vector3f AB = points[0] - points[1];
                    Vector3f AC = points[0] - points[2];
                    Vector3f normal = glm::normalize(glm::cross(AB, AC));
                    normals.emplace_back(normal);
                    normals.emplace_back(normal);
                    normals.emplace_back(normal);
                }
            }

            std::vector<Wave> m_waves;
        };
        scene->Create<NativeScriptComponent>(ocean).Bind<OceanScript>();
    }
#pragma endregion SETUP_OCEAN

#pragma region SETUP_SKY
    constexpr float TWO_PI = 2.0f * glm::pi<float>();
    auto create_cloud = [&](int p_index, ecs::Entity p_parent) {
        int block = 3 + static_cast<int>(glm::floor(Random::Float() * 3));
        for (int block_index = 0; block_index < block; ++block_index) {
            std::string name = std::format("block_{}_{}", p_index, block_index);
            const float scale = 0.6f + Random::Float() * 0.4f;
            auto cloud = scene->CreateCubeEntity(name, material_white, Vector3f(5.0f * scale));
            auto transform = scene->GetComponent<TransformComponent>(cloud);
            Vector3f position(block_index * 8.0f, Random::Float() * 6.0f, Random::Float() * 6.0f);
            Vector3f rotation(0.0f, TWO_PI * Random::Float(), TWO_PI * Random::Float());
            transform->Rotate(rotation);
            transform->SetTranslation(position);

            scene->AttachChild(cloud, p_parent);
        }
    };

    constexpr float step_angle = TWO_PI / CLOUD_COUNT;
    for (int cloud_index = 0; cloud_index < CLOUD_COUNT; ++cloud_index) {
        const float angle = step_angle * cloud_index;
        std::string name = std::format("cloud_{}", cloud_index);
        auto cloud = scene->CreateTransformEntity(name);
        scene->AttachChild(cloud, earth);

        auto transform = scene->GetComponent<TransformComponent>(cloud);
        const float x = glm::sin(angle) * (OCEAN_RADIUS + 40.0f);
        const float y = glm::cos(angle) * (OCEAN_RADIUS + 40.0f);
        Matrix4x4f matrix = glm::translate(Vector3f(x, y, -50.0f)) * glm::rotate(angle, Vector3f(0, 0, -1));
        transform->SetLocalTransform(matrix);
        create_cloud(cloud_index, cloud);
    }
#pragma endregion SETUP_SKY

    {
        auto sky_light = scene->CreateHemisphereLightEntity("sky_light", "@res://images/ibl/sky.hdr");
        scene->AttachChild(sky_light, root);
    }

    return scene;
}

}  // namespace my
