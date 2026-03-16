#if 0

#include <VulkanRenderer/Animator.hpp>

#include <chrono>
#include <glm/gtx/matrix_interpolation.hpp>

#include <algorithm>
#include <print>
#include <ranges>

Animator::Animator() {}

void Animator::UpdateAnimation(float dt) {
  m_DeltaTime = dt;
  if (m_CurrentAnimation) {
    m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
    m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
    CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
  }
}

void Animator::SetAnimationTime(float t) {
  m_DeltaTime = t;
  if (m_CurrentAnimation) {
    m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
    CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
  }
}

void Animator::PlayAnimation(Animation *pAnimation) {
  m_CurrentAnimation = pAnimation;
  m_CurrentTime = 0.0f;
  m_FinalBoneMatrices.resize(pAnimation->m_Bones.size(), glm::mat4(1.0f));
}

void Animator::CalculateBoneTransform(const AssimpNodeData *node,
                                      glm::mat4 parentTransform) {
  std::string nodeName = node->name;
  glm::mat4 nodeTransform = node->transformation;

  Bone *Bone = m_CurrentAnimation->FindBone(nodeName);
  if (Bone) {
    Bone->Update(m_CurrentTime);
    nodeTransform = Bone->GetLocalTransform();
  }

  glm::mat4 globalTransformation = parentTransform * nodeTransform;
  auto& boneInfoMap = m_CurrentAnimation->GetBoneInfos().get();

  if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
    int index = boneInfoMap[nodeName].id;
    glm::mat4 offset = boneInfoMap[nodeName].offset;
    m_FinalBoneMatrices[index] = globalTransformation * offset;
  }

  for (int i = 0; i < node->children.size(); i++)
    CalculateBoneTransform(&node->children[i], globalTransformation);
}

auto Animator::GetFinalBoneMatrices() -> std::span<glm::mat4> {
  return m_FinalBoneMatrices;
}

namespace animation {

#if 0
Axis::Axis(ValueType v) : m_value{std::clamp(v, -1.0f, 1.0f)} {}
auto Axis::get() const noexcept -> ValueType { return m_value; }
auto Axis::min() noexcept -> Axis { return Axis(-1.0f); }
auto Axis::max() noexcept -> Axis { return Axis(1.0f); }
auto Axis::zero() noexcept -> Axis { return Axis(0.0f); }
#endif

[[nodiscard]]
auto animate(Animation &animation, float time)
    -> std::optional<std::vector<glm::mat4>> {
  Animator animator;
  animator.PlayAnimation(&animation);
  animator.UpdateAnimation(time);
  return animator.GetFinalBoneMatrices() | std::ranges::to<std::vector>();
}

BlendAnimator::BlendAnimator()
    : m_base_animation{nullptr}, m_layered_animation{nullptr},
      m_CurrentTime{0.0f}, m_DeltaTime{0.0f} {}

BlendAnimator::BlendAnimator(Animation *base_animation,
                             Animation *layered_animation)
    : m_base_animation{base_animation}, m_layered_animation{layered_animation},
      m_CurrentTime{0.0f}, m_DeltaTime{0.0f} {
  m_FinalBoneMatrices.resize(base_animation->m_Bones.size(), glm::mat4(1.0f));
}

auto BlendAnimator::Update(float dt, Bias blend_factor)
    -> std::span<glm::mat4> {
  m_DeltaTime = dt;
  m_CurrentTime += m_base_animation->GetTicksPerSecond() * dt;
  m_CurrentTime = fmod(m_CurrentTime, m_base_animation->GetDuration());
  CalculateBlendedBoneTransform(&m_base_animation->GetRootNode(),
                                &m_layered_animation->GetRootNode(),
                                glm::mat4(1.0f), blend_factor);
  return GetFinalBoneMatrices();
}

void BlendAnimator::CalculateBlendedBoneTransform(
    const AssimpNodeData *node, const AssimpNodeData *nodeLayered,
    const glm::mat4 &parentTransform, Bias blend_factor) {
  const std::string &nodeName = node->name;

  glm::mat4 nodeTransform = node->transformation;
  Bone *pBone = m_base_animation->FindBone(nodeName);
  if (pBone) {
    pBone->Update(m_CurrentTime);
    nodeTransform = pBone->GetLocalTransform();
  }

  glm::mat4 layeredNodeTransform = nodeLayered->transformation;
  pBone = m_layered_animation->FindBone(nodeName);
  if (pBone) {
    pBone->Update(m_CurrentTime);
    layeredNodeTransform = pBone->GetLocalTransform();
  }

  // Blend two matrices
  const glm::quat rot0 = glm::quat_cast(nodeTransform);
  const glm::quat rot1 = glm::quat_cast(layeredNodeTransform);
  const glm::quat finalRot = glm::slerp(rot0, rot1, blend_factor.get());
  glm::mat4 blendedMat = glm::mat4_cast(finalRot);
  blendedMat[3] = (1.0f - blend_factor.get()) * nodeTransform[3] +
                  layeredNodeTransform[3] * blend_factor.get();

  const glm::mat4 globalTransformation = parentTransform * blendedMat;

  const std::map<std::string, BoneInfo> &boneInfoMap =
      m_base_animation->GetBoneInfos().get();

  if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
    const int index = boneInfoMap.at(nodeName).id;
    const glm::mat4 &offset = boneInfoMap.at(nodeName).offset;
    const glm::mat4 &offsetLayerMat =
        m_layered_animation->GetBoneInfos().get().at(nodeName).offset;

    // Blend two matrices... again
    const glm::quat rot0 = glm::quat_cast(offset);
    const glm::quat rot1 = glm::quat_cast(offsetLayerMat);
    const glm::quat finalRot = glm::slerp(rot0, rot1, blend_factor.get());
    glm::mat4 blendedMat = glm::mat4_cast(finalRot);
    blendedMat[3] = (1.0f - blend_factor.get()) * offset[3] +
                    offsetLayerMat[3] * blend_factor.get();

    m_FinalBoneMatrices[index] = globalTransformation * blendedMat;
  }

  for (size_t i = 0; i < node->children.size(); ++i)
    CalculateBlendedBoneTransform(&node->children[i], &nodeLayered->children[i],
                                  globalTransformation, blend_factor);
}

auto BlendAnimator::GetFinalBoneMatrices() -> std::span<glm::mat4> {
  return m_FinalBoneMatrices;
}

} // namespace animation

#endif
