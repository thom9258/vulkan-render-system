#if 0

#pragma once

#include "Animation.hpp"
#include "Bias.hpp"

#include <chrono>
#include <span>

// https://stackoverflow.com/questions/69860756/how-do-i-correctly-blend-between-skeletal-animations-in-opengl-from-a-walk-anima

class Animator {
public:
  Animator();
  void SetAnimationTime(float t);
  void UpdateAnimation(float dt);
  void PlayAnimation(Animation *pAnimation);
  auto GetFinalBoneMatrices() -> std::span<glm::mat4>;

private:
  void CalculateBoneTransform(const AssimpNodeData *node,
                              glm::mat4 parentTransform);
  std::vector<glm::mat4> m_FinalBoneMatrices;
  Animation *m_CurrentAnimation;
  float m_CurrentTime;
  float m_DeltaTime;
};

namespace animation {

#if 0
class Axis {
public:
  using ValueType = float;
  explicit Axis(ValueType v);
  ~Axis() = default;
  Axis(const Axis &) = default;
  Axis(Axis &&) = default;
  Axis &operator=(const Axis &) = default;
  Axis &operator=(Axis &&) = default;
  auto get() const noexcept -> ValueType;

  static auto min() noexcept -> Axis;
  static auto max() noexcept -> Axis;
  static auto zero() noexcept -> Axis;

private:
  ValueType m_value;
};

struct Axis2D {
	Axis x;
	Axis y;
};

struct RingAnimator2D {
	Bias size;
  	Animator front;
	Animator back;
	Animator left;
	Animator right;
};

struct RingAnimatorSingle {
	Bias size;
  	Animator front;
	Animator back;
	Animator left;
	Animator right;
};    

struct CharacterAxisInterpolator {
  CharacterAxisInterpolator(RingAnimatorSingle&& center, std::vector<RingAnimator2D> rings);

[[nodiscard]]
auto interpolate()
    -> std::optional<std::vector<glm::mat4>>;
};

#endif
	


[[nodiscard]]
auto animate(Animation animation, float time)
    -> std::optional<std::vector<glm::mat4>>;

class BlendAnimator {
public:
  BlendAnimator();
  BlendAnimator(Animation *base_animation, Animation *layered_animation);
  auto Update(float dt, Bias blend_factor) -> std::span<glm::mat4>;
  auto GetFinalBoneMatrices() -> std::span<glm::mat4>;

private:
  void CalculateBlendedBoneTransform(const AssimpNodeData *node,
                                     const AssimpNodeData *nodeLayered,
                                     const glm::mat4 &parentTransform,
                                     Bias blend_factor);

  Animation *m_base_animation;
  Animation *m_layered_animation;
  float m_CurrentTime;
  float m_DeltaTime;
  std::vector<glm::mat4> m_FinalBoneMatrices;
};

} // namespace animation

#endif
