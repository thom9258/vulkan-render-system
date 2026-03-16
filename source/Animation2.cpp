#include <VulkanRenderer/Animation2.hpp>

#include <glm/gtx/matrix_interpolation.hpp>

#include <algorithm>
#include <cmath>
#include <print>
#include <ranges>

namespace animation {

auto BoneInfos::get() -> std::map<std::string, BoneInfo> & {
  return m_BoneInfoMap;
}

auto BoneInfos::has_bone(std::string_view name) -> bool {
  return get().find(std::string(name)) != get().end();
}

auto BoneInfos::generate_next_id() -> int {
  int id = m_id_counter;
  m_id_counter++;
  return id;
}

auto BoneInfos::insert_bone(std::string_view name, glm::mat4 offset) -> int {

  if (has_bone(name))
    return get().at(std::string(name)).id;
  int id = generate_next_id();
  get()[std::string(name)] = {id, offset};
  return id;
}

auto BoneInfos::find_bone_id(std::string_view name) -> std::optional<int> {
  if (!has_bone(name))
    return std::nullopt;
  return get().at(std::string(name)).id;
}

auto BoneInfos::find_bone(std::string_view name) -> std::optional<BoneInfo> {
  if (!has_bone(name))
    return std::nullopt;
  return get().at(std::string(name));
}

auto BoneInfos::bone_count() const -> std::size_t {
  return m_BoneInfoMap.size();
}

Bone::Bone(std::string_view name, int ID) : m_name(name), m_ID(ID) {}

auto Bone::animate(double time) -> std::optional<glm::mat4> {
  std::optional<glm::mat4> translation = interpolate_position(time);
  if (!translation.has_value())
    return std::nullopt;

  std::optional<glm::mat4> rotation = interpolate_rotation(time);
  if (!rotation.has_value())
    return std::nullopt;

  std::optional<glm::mat4> scale = interpolate_scale(time);
  if (!scale.has_value())
    return std::nullopt;

  return translation.value() * rotation.value() * scale.value();
}

auto Bone::name() -> std::string_view { return m_name; }

auto Bone::id() const -> int { return m_ID; }

auto Bone::position_index(double time) -> std::optional<std::size_t> {
  for (std::size_t i = 0; i < m_Positions.size() - 1; ++i) {
    if (time < m_Positions[i + 1].timeStamp) {
      return i;
    }
  }

  return std::nullopt;
}

auto Bone::rotation_index(double time) -> std::optional<std::size_t> {
  for (std::size_t i = 0; i < m_Rotations.size() - 1; ++i) {
    if (time < m_Rotations[i + 1].timeStamp) {
      return i;
    }
  }

  return std::nullopt;
}

/* Gets the current index on mKeyScalings to interpolate to based on the
current animation time */
auto Bone::scale_index(double time) -> std::optional<std::size_t> {
  for (std::size_t i = 0; i < m_Scales.size() - 1; ++i) {
    if (time < m_Scales[i + 1].timeStamp) {
      return i;
    }
  }

  return std::nullopt;
}

auto Bone::get_bias_between_keyframes(double lastTimeStamp,
                                      double nextTimeStamp,
                                      double animationTime) -> Bias {

  double midWayLength = animationTime - lastTimeStamp;
  double framesDiff = nextTimeStamp - lastTimeStamp;
  Bias bias(midWayLength / framesDiff);
  if (bias.get() != midWayLength / framesDiff) {
    std::println("Funky Bias calculation between keyframes, with value {}",
                 midWayLength / framesDiff);
  }

  return bias;
}

auto Bone::interpolate_position(double time) -> std::optional<glm::mat4> {
  if (1 == m_Positions.size())
    return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

  std::optional<std::size_t> keyframe0 = position_index(time);
  if (!keyframe0.has_value()) {
    std::println("ERROR: INTERPOLATE POS HAD NO KEYFRAME");
    return std::nullopt;
  }

  std::size_t keyframe1 = keyframe0.value() + 1;
  Bias keyframe_bias =
      get_bias_between_keyframes(m_Positions[keyframe0.value()].timeStamp,
                                 m_Positions[keyframe1].timeStamp, time);

  glm::vec3 position =
      glm::mix(m_Positions[keyframe0.value()].position,
               m_Positions[keyframe1].position, keyframe_bias.get());

  return glm::translate(glm::mat4(1.0f), position);
}

auto Bone::interpolate_rotation(double time) -> std::optional<glm::mat4> {
  if (1 == m_Rotations.size()) {
    auto rotation = glm::normalize(m_Rotations[0].orientation);
    return glm::toMat4(rotation);
  }

  std::optional<std::size_t> keyframe0 = rotation_index(time);
  if (!keyframe0.has_value()) {
    std::println("ERROR: INTERPOLATE ROT HAD NO KEYFRAME");
    return std::nullopt;
  }

  std::size_t keyframe1 = keyframe0.value() + 1;
  Bias keyframe_bias =
      get_bias_between_keyframes(m_Rotations[keyframe0.value()].timeStamp,
                                 m_Rotations[keyframe1].timeStamp, time);

  glm::quat rotation = glm::slerp(m_Rotations[keyframe0.value()].orientation,
                                  m_Rotations[keyframe1].orientation,
                                  static_cast<float>(keyframe_bias.get()));

  return glm::toMat4(glm::normalize(rotation));
}

auto Bone::interpolate_scale(double time) -> std::optional<glm::mat4> {
  if (1 == m_Scales.size())
    return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

  std::optional<std::size_t> keyframe0 = scale_index(time);
  if (!keyframe0.has_value()) {
    std::println("ERROR: INTERPOLATE SCALE HAD NO KEYFRAME");
    return std::nullopt;
  }

  std::size_t keyframe1 = keyframe0.value() + 1;
  Bias keyframe_bias =
      get_bias_between_keyframes(m_Scales[keyframe0.value()].timeStamp,
                                 m_Scales[keyframe1].timeStamp, time);

  glm::vec3 scale = glm::mix(m_Scales[keyframe0.value()].scale,
                             m_Scales[keyframe1].scale, keyframe_bias.get());

  return glm::scale(glm::mat4(1.0f), scale);
}

Skeleton::Skeleton(std::string_view name, glm::mat4 model_matrix)
    : m_name{std::string(name)}, m_model_matrix{model_matrix} {}

void Skeleton::add_child(Skeleton &&child) {
  m_children.push_back(std::move(child));
}

std::span<Skeleton> Skeleton::children() { return m_children; }

std::string_view Skeleton::name() { return m_name; }

glm::mat4 Skeleton::model_matrix() { return m_model_matrix; }

void Skeleton::set_model_matrix(glm::mat4 matrix) { m_model_matrix = matrix; }

[[nodiscard]]
auto copy(Skeleton &skeleton) -> Skeleton {
  Skeleton ret(skeleton.name(), skeleton.model_matrix());
  for (Skeleton &child : skeleton.children()) {
    ret.add_child(copy(child));
  }

  return ret;
}

Animation::Animation(Skeleton skeleton, TotalTicks total_ticks,
                     TicksPerSecond ticks_per_second)
    : m_initial_pose(std::move(skeleton)), m_total_ticks{total_ticks},
      m_ticks_per_second{ticks_per_second} {}

auto Animation::ticks_per_second() const -> TicksPerSecond {
  return m_ticks_per_second;
}

auto Animation::total_ticks() const -> TotalTicks { return m_total_ticks; }
auto Animation::set_ticks_per_second(TicksPerSecond ticks) {
  m_ticks_per_second = ticks;
}

auto Animation::add_bone(Bone bone) -> void { m_bones.push_back(bone); }

auto Animation::find_bone(std::string_view name) -> Bone * {
  auto iter = std::find_if(m_bones.begin(), m_bones.end(),
                           [&](Bone &bone) { return bone.name() == name; });

  if (iter == m_bones.end()) {
    return nullptr;
  } else {
    return &(*iter);
  }
}

auto Animation::bone_infos() -> BoneInfos & { return m_bone_infos; }

void Animation::set_bone_infos(BoneInfos &infos) { m_bone_infos = infos; }

auto Animation::animate(double time) -> std::optional<Skeleton> {
  time = std::fmod(time * m_ticks_per_second.get(), m_total_ticks.get());
  bool found_invalid_bone = false;
  const auto get_animated_bone_matrix = [&](auto recurse, Skeleton &skeleton) {
    if (found_invalid_bone) {
      return;
    }

    if (Bone *bone = find_bone(skeleton.name())) {
      std::optional<glm::mat4> animated_bone_matrix = bone->animate(time);
      if (!animated_bone_matrix.has_value()) {
        std::println("Could not calculate animated bone by name {}",
                     skeleton.name());
        found_invalid_bone = true;
        return;
      }

      skeleton.set_model_matrix(animated_bone_matrix.value());
    }

    for (Skeleton &child : skeleton.children()) {
      recurse(recurse, child);
    }
  };

  Skeleton animated = copy(m_initial_pose);
  get_animated_bone_matrix(get_animated_bone_matrix, animated);
  if (found_invalid_bone) {
    std::println("ERROR: DID INVALID OPERATION IN ANIMATION");
    return std::nullopt;
  }

  return animated;
}

std::optional<Skeleton> blend_skeletons(Skeleton &a, Skeleton &b, Bias bias) {
  if (a.name() != b.name()) {
    return std::nullopt;
  }

  if (a.children().size() != b.children().size()) {
    return std::nullopt;
  }

  Skeleton blended_skeleton(a.name(),
                            glm::interpolate(a.model_matrix(), b.model_matrix(),
                                             static_cast<float>(bias.get())));

  for (auto [child_a, child_b] : std::views::zip(a.children(), b.children())) {
    auto blended_child = blend_skeletons(child_a, child_b, bias);
    if (!blended_child.has_value())
      return std::nullopt;

    blended_skeleton.add_child(std::move(blended_child.value()));
  }

  return blended_skeleton;
}

FinalAnimationState::FinalAnimationState(std::size_t matrice_count) {
  m_matrices.resize(matrice_count, glm::mat4(1.0f));
}

void FinalAnimationState::insert(std::size_t i, const glm::mat4 &final_matrix) {
  m_matrices.at(i) = final_matrix;
}

std::span<glm::mat4> FinalAnimationState::matrices() { return m_matrices; }

auto insert_final_matrices(FinalAnimationState &final_state,
                           BoneInfos &bone_infos, Skeleton &bone,
                           glm::mat4 parent_matrix) -> void {

  glm::mat4 current_matrix = parent_matrix;

  std::optional<BoneInfo> info = bone_infos.find_bone(bone.name());
  if (info.has_value()) {
    // Note here that when we convert from local to global transform, we
    // multiply parent * current * offset
    // but when we propagate to the children, we only pass parent * current
	// as the parent of the children.
    glm::mat4 offset = info.value().offset;
    current_matrix = parent_matrix * bone.model_matrix();
    final_state.insert(info.value().id, current_matrix * offset);
  }

  for (Skeleton &child : bone.children()) {
    insert_final_matrices(final_state, bone_infos, child, current_matrix);
  }
}

auto calculate_final_animation_state(BoneInfos &bone_infos, Skeleton &skeleton)
    -> std::optional<FinalAnimationState> {
  FinalAnimationState final_state(bone_infos.bone_count());
  insert_final_matrices(final_state, bone_infos, skeleton, glm::mat4(1.0f));
  return final_state;
}

} // namespace animation
