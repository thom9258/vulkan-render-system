#include <VulkanRenderer/Animation.hpp>

#include <algorithm>
#include <print>

auto BoneInfos::get() -> std::map<std::string, BoneInfo> & {
  return m_BoneInfoMap;
}

auto BoneInfos::counter() -> int & { return m_BoneCounter; }

auto BoneInfos::has_bone(std::string_view name) -> bool {
  return get().find(std::string(name)) != get().end();
}

auto BoneInfos::insert_bone(std::string_view name, glm::mat4 offset) -> int {

  if (has_bone(name))
    return get().at(std::string(name)).id;
  int id = counter();
  get()[std::string(name)] = {id, offset};
  counter()++;
  return id;
}

auto BoneInfos::find_bone_id(std::string_view name) -> std::optional<int> {
  if (!has_bone(name))
    return std::nullopt;
  return get().at(std::string(name)).id;
}

Bone::Bone(const std::string &name, int ID)
    : m_Name(name), m_ID(ID), m_LocalTransform(1.0f) {}

/*interpolates  b/w positions,rotations & scaling keys based on the curren time
of the animation and prepares the local transformation matrix by combining all
keys tranformations*/
void Bone::Update(float animationTime) {
  glm::mat4 translation = InterpolatePosition(animationTime);
  glm::mat4 rotation = InterpolateRotation(animationTime);
  glm::mat4 scale = InterpolateScaling(animationTime);
  m_LocalTransform = translation * rotation * scale;
}

glm::mat4 Bone::GetLocalTransform() { return m_LocalTransform; }
std::string Bone::GetBoneName() const { return m_Name; }
int Bone::GetBoneID() { return m_ID; }

/* Gets the current index on mKeyPositions to interpolate to based on
the current animation time*/
int Bone::GetPositionIndex(float animationTime) {
  for (int i = 0; i < m_Positions.size() - 1; ++i) {
    if (animationTime < m_Positions[i + 1].timeStamp)
      return i;
  }
  assert(0);
  return 0;
}

/* Gets the current index on mKeyRotations to interpolate to based on the
current animation time*/
int Bone::GetRotationIndex(float animationTime) {
  for (int i = 0; i < m_Rotations.size() - 1; ++i) {
    if (animationTime < m_Rotations[i + 1].timeStamp)
      return i;
  }
  assert(0);
  return 0;
}

/* Gets the current index on mKeyScalings to interpolate to based on the
current animation time */
int Bone::GetScaleIndex(float animationTime) {
  for (int i = 0; i < m_Scales.size() - 1; ++i) {
    if (animationTime < m_Scales[i + 1].timeStamp)
      return i;
  }
  assert(0);
  return 0;
}

/* Gets normalized value for Lerp & Slerp*/
float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp,
                           float animationTime) {
  float scaleFactor = 0.0f;
  float midWayLength = animationTime - lastTimeStamp;
  float framesDiff = nextTimeStamp - lastTimeStamp;
  scaleFactor = midWayLength / framesDiff;
  return scaleFactor;
}

/*figures out which position keys to interpolate b/w and performs the
interpolation and returns the translation matrix*/
glm::mat4 Bone::InterpolatePosition(float animationTime) {
  if (1 == m_Positions.size())
    return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

  int p0Index = GetPositionIndex(animationTime);
  int p1Index = p0Index + 1;
  float scaleFactor =
      GetScaleFactor(m_Positions[p0Index].timeStamp,
                     m_Positions[p1Index].timeStamp, animationTime);
  glm::vec3 finalPosition =
      glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position,
               scaleFactor);
  return glm::translate(glm::mat4(1.0f), finalPosition);
}

/*figures out which rotations keys to interpolate b/w and performs the
interpolation and returns the rotation matrix*/
glm::mat4 Bone::InterpolateRotation(float animationTime) {
  if (1 == m_Rotations.size()) {
    auto rotation = glm::normalize(m_Rotations[0].orientation);
    return glm::toMat4(rotation);
  }

  int p0Index = GetRotationIndex(animationTime);
  int p1Index = p0Index + 1;
  float scaleFactor =
      GetScaleFactor(m_Rotations[p0Index].timeStamp,
                     m_Rotations[p1Index].timeStamp, animationTime);
  glm::quat finalRotation =
      glm::slerp(m_Rotations[p0Index].orientation,
                 m_Rotations[p1Index].orientation, scaleFactor);
  finalRotation = glm::normalize(finalRotation);
  return glm::toMat4(finalRotation);
}

/*figures out which scaling keys to interpolate b/w and performs the
interpolation and returns the scale matrix*/
glm::mat4 Bone::InterpolateScaling(float animationTime) {
  if (1 == m_Scales.size())
    return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

  int p0Index = GetScaleIndex(animationTime);
  int p1Index = p0Index + 1;
  float scaleFactor = GetScaleFactor(
      m_Scales[p0Index].timeStamp, m_Scales[p1Index].timeStamp, animationTime);

  glm::vec3 finalScale =
      glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
  return glm::scale(glm::mat4(1.0f), finalScale);
}

Bone *Animation::FindBone(const std::string &name) {
  auto iter =
      std::find_if(m_Bones.begin(), m_Bones.end(), [&](const Bone &Bone) {
        return Bone.GetBoneName() == name;
      });
  if (iter == m_Bones.end())
    return nullptr;
  else
    return &(*iter);
}

float Animation::GetTicksPerSecond() { return m_TicksPerSecond; }

float Animation::GetDuration() { return m_Duration; }

const AssimpNodeData &Animation::GetRootNode() { return m_RootNode; }

const std::map<std::string, BoneInfo> &Animation::GetBoneIDMap() {
  return m_BoneInfoMap;
}

Animator::Animator() { m_FinalBoneMatrices.resize(100, glm::mat4(1.0f)); }

void Animator::UpdateAnimation(float dt) {
  m_DeltaTime = dt;
  if (m_CurrentAnimation) {
    m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
    m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
    CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
  }
}

void Animator::PlayAnimation(Animation *pAnimation) {
  m_CurrentAnimation = pAnimation;
  m_CurrentTime = 0.0f;
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
  auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();

  if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
    int index = boneInfoMap[nodeName].id;
    glm::mat4 offset = boneInfoMap[nodeName].offset;
    m_FinalBoneMatrices[index] = globalTransformation * offset;
  }

  for (int i = 0; i < node->children.size(); i++)
    CalculateBoneTransform(&node->children[i], globalTransformation);
}

//TODO: copy being made here...
std::vector<glm::mat4> Animator::GetFinalBoneMatrices() {
  return m_FinalBoneMatrices;
}
