#pragma once

#include "Bias.hpp"
#include "glm.hpp"
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <span>

namespace animation {

namespace keyframe {

struct Position {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  double timeStamp{0};
};

struct Rotation {
  glm::quat orientation{0.0f, 0.0f, 0.0f, 1.0f};
  double timeStamp{0};
};

struct Scale {
  glm::vec3 scale{1.0f, 1.0f, 1.0f};
  double timeStamp{0};
};

} // namespace keyframe

// TODO: we are just making shit public because i didnt bother to fix public
// private stuff.. we need this because the constructors are pure ffunctions in
// private
class Bone {
public:
  std::vector<keyframe::Position> m_Positions;
  std::vector<keyframe::Rotation> m_Rotations;
  std::vector<keyframe::Scale> m_Scales;

  Bone(std::string_view name, int ID);
  auto name() -> std::string_view;
  auto animate(double time) -> std::optional<glm::mat4>;
  auto id() const -> int;

private:
  auto position_index(double time)
      -> std::optional<std::size_t>;
  auto rotation_index(double time)
      -> std::optional<std::size_t>;
  auto scale_index(double time)
      -> std::optional<std::size_t>;

  auto get_bias_between_keyframes(double lastTimeStamp,
                                  double nextTimeStamp,
                                  double animationTime)
      -> Bias;

  auto interpolate_position(double time)
      -> std::optional<glm::mat4>;
  auto interpolate_rotation(double time)
      -> std::optional<glm::mat4>;
  auto interpolate_scale(double time)
      -> std::optional<glm::mat4>;

  std::string m_name;
  int m_ID;
};

struct BoneInfo {
  int id{-1};
  glm::mat4 offset{glm::mat4(1.0f)};
};

class BoneInfos {
public:
  auto has_bone(std::string_view name) -> bool;
  auto insert_bone(std::string_view name, glm::mat4 offset) -> int;
  auto find_bone_id(std::string_view name) -> std::optional<int>;
  auto find_bone(std::string_view name) -> std::optional<BoneInfo>;
  auto get() -> std::map<std::string, BoneInfo> &;
  auto bone_count() const -> std::size_t;

private:
  auto generate_next_id() -> int;
  std::map<std::string, BoneInfo> m_BoneInfoMap;
  int m_id_counter = 0;
};

class Skeleton {
public:
  Skeleton(std::string_view name, glm::mat4 model_matrix);
  Skeleton(const Skeleton &) = delete;
  Skeleton &operator=(const Skeleton &) = delete;
  Skeleton(Skeleton &&) = default;
  Skeleton &operator=(Skeleton &&) = default;

  void add_child(Skeleton &&child);
  std::span<Skeleton> children();
  std::string_view name();
  glm::mat4 model_matrix();
  void set_model_matrix(glm::mat4 matrix);

private:
  std::string m_name;
  glm::mat4 m_model_matrix;
  std::vector<Skeleton> m_children;
};

[[nodiscard]]
auto copy(Skeleton &skeleton) -> Skeleton;

class Animation {
public:
  Animation(Skeleton skeleton, TotalTicks total_ticks, TicksPerSecond ticks_per_second);
  Animation(Animation &&) = default;
  Animation &operator=(Animation &&) = default;
  Animation(const Animation &) = delete;
  Animation &operator=(const Animation &) = delete;
  ~Animation() = default;

  auto add_bone(Bone bone) -> void;
  auto find_bone(std::string_view name) -> Bone *;
  auto ticks_per_second() const -> TicksPerSecond;
  auto total_ticks() const -> TotalTicks;
  auto set_ticks_per_second(TicksPerSecond ticks);
  auto bone_infos() -> BoneInfos &;
  void set_bone_infos(BoneInfos &infos);

  [[nodiscard]]
  auto animate(double time) -> std::optional<Skeleton>;

  Skeleton m_initial_pose;
  BoneInfos m_bone_infos;
  std::vector<Bone> m_bones;

private:
  TotalTicks m_total_ticks;
  TicksPerSecond m_ticks_per_second;
};

std::optional<Skeleton> blend_skeletons(Skeleton &a, Skeleton &b, Bias bias);

class FinalAnimationState {
public:
  FinalAnimationState(std::size_t matrice_count);
  FinalAnimationState(const FinalAnimationState &) = delete;
  FinalAnimationState &operator=(const FinalAnimationState &) = delete;
  FinalAnimationState(FinalAnimationState &&) = default;
  FinalAnimationState &operator=(FinalAnimationState &&) = default;

  void insert(std::size_t i, const glm::mat4 &final_matrix);
  std::span<glm::mat4> matrices();

private:
  std::vector<glm::mat4> m_matrices;
};

auto insert_final_matrices(FinalAnimationState &final_state,
                           BoneInfos &bone_infos, bool &found_invalid_bone,
                           Skeleton &bone, glm::mat4 parent_matrix) -> void;

auto calculate_final_animation_state(BoneInfos &bone_infos, Skeleton &skeleton)
    -> std::optional<FinalAnimationState>;

} // namespace animation
