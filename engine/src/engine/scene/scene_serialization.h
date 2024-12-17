#pragma once

namespace my {

class Scene;

[[nodiscard]] Result<void> SaveSceneBinary(const std::string& p_path, Scene& p_scene);
[[nodiscard]] Result<void> LoadSceneBinary(const std::string& p_path, Scene& p_scene);

[[nodiscard]] Result<void> SaveSceneText(const std::string& p_path, const Scene& p_scene);
[[nodiscard]] Result<void> LoadSceneText(const std::string& p_path, Scene& p_scene);

}  // namespace my
