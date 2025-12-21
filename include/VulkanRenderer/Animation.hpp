#pragma once

#include "glm.hpp"
#include <map>
#include <optional>
#include <string>
#include <vector>


struct BoneInfo
{
    int id;
    glm::mat4 offset;
};

class BoneInfos {
public:
	auto has_bone(std::string_view name) -> bool;
	auto insert_bone(std::string_view name, glm::mat4 offset) -> int;
	auto find_bone_id(std::string_view name) -> std::optional<int>;
	auto get() -> std::map<std::string, BoneInfo>&;

private:
	auto counter() -> int&;
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;
};

struct KeyPosition
{
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation
{
    glm::quat orientation;
    float timeStamp;
};

struct KeyScale
{
    glm::vec3 scale;
    float timeStamp;
};


// TODO: we are just making shit public because i didnt bother to fix public
// private stuff.. we need this because the constructors are pure ffunctions in private
class Bone
{
public:
    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale> m_Scales;
    int m_NumPositions;
    int m_NumRotations;
    int m_NumScalings;
	
    glm::mat4 m_LocalTransform;
    std::string m_Name;
    int m_ID;

    Bone(const std::string& name, int ID);
    void Update(float animationTime);

    glm::mat4 GetLocalTransform();
    std::string GetBoneName() const;
    int GetBoneID();
    int GetPositionIndex(float animationTime);
    int GetRotationIndex(float animationTime);
    int GetScaleIndex(float animationTime);

private:
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
    glm::mat4 InterpolatePosition(float animationTime);
    glm::mat4 InterpolateRotation(float animationTime);
    glm::mat4 InterpolateScaling(float animationTime);
};

struct AssimpNodeData
{
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

// TODO: we are just making shit public because i didnt bother to fix public
// private stuff.. we need this because the constructors are pure ffunctions in private
class Animation
{
public:
    Animation() = default;
    ~Animation() = default;

    Bone* FindBone(const std::string& name);
    float GetTicksPerSecond();
    float GetDuration(); 
    const AssimpNodeData& GetRootNode();
    const std::map<std::string,BoneInfo>& GetBoneIDMap() ;
    float m_Duration;
    int m_TicksPerSecond;
    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;
    std::map<std::string, BoneInfo> m_BoneInfoMap;
};


class Animator
{	
public:
    Animator();
    void UpdateAnimation(float dt);
    void PlayAnimation(Animation* pAnimation);
    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);
    std::vector<glm::mat4> GetFinalBoneMatrices();
		
private:
    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation;
    float m_CurrentTime;
    float m_DeltaTime;	
};
