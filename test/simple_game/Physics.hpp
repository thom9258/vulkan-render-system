#pragma once

#include <VulkanRenderer/Mesh.hpp>

#include "LinearMath/btIDebugDraw.h"
#include "btBulletDynamicsCommon.h"

#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
//#include <BulletCollision/CollisionDispatch/btRaycastCallback.h>

#include <memory>
#include <vector>
#include <print>

namespace physics {

class DebugLineCollecter : public btIDebugDraw {
  int m_debugMode;

public:
	
  std::vector<VertexPosNormColorUV> debug_lines;

  DebugLineCollecter() = default;
  virtual ~DebugLineCollecter() = default;

  void clear() { debug_lines.clear(); }

  void drawLine(const btVector3 &from, const btVector3 &to,
                const btVector3 &fromColor, const btVector3 &toColor) override {
    drawLine(from, to, fromColor);
  }

  void drawLine(const btVector3 &from, const btVector3 &to,
                const btVector3 &color) override {
	VertexPosNormColorUV a;
    a.pos = glm::vec3(from.x(), from.y(), from.z());
    a.norm = glm::vec3(0.0f);
    a.color = glm::vec3(color.x(), color.y(), color.z());
    a.uv = glm::vec2(0.0f);

	VertexPosNormColorUV b;
    b.pos = glm::vec3(to.x(), to.y(), to.z());
    b.norm = glm::vec3(0.0f);
    b.color = glm::vec3(color.x(), color.y(), color.z());
    b.uv = glm::vec2(0.0f);

    debug_lines.push_back(a);
    debug_lines.push_back(b);
    debug_lines.push_back(a);
  }

  void drawSphere(const btVector3 &p, btScalar radius,
                  const btVector3 &color) override {}

  void drawTriangle(const btVector3 &a, const btVector3 &b, const btVector3 &c,
                    const btVector3 &color, btScalar alpha) override {}

  void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB,
                        btScalar distance, int lifeTime,
                        const btVector3 &color) override {}

  void reportErrorWarning(const char *warningString) override { std::println("Bullet3 Physics: {}", warningString); }

  void draw3dText(const btVector3 &location, const char *textString) override {}

  void setDebugMode(int debugMode) override { m_debugMode = debugMode; }

  int getDebugMode() const override { return m_debugMode; }
};

struct Physics {
  std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration{
      nullptr};
  std::unique_ptr<btCollisionDispatcher> dispatcher{nullptr};
  std::unique_ptr<btBroadphaseInterface> overlappingPairCache{nullptr};
  std::unique_ptr<btSequentialImpulseConstraintSolver> solver{nullptr};
  std::unique_ptr<DebugLineCollecter> debug_line_collecter{nullptr};
  std::unique_ptr<btDiscreteDynamicsWorld> dynamicsWorld{nullptr};

  Physics() {
    /// collision configuration contains default setup for memory, collision
    /// setup. Advanced users can create their own configuration.
    collisionConfiguration =
        std::make_unique<btDefaultCollisionConfiguration>();

    /// use the default collision dispatcher. For parallel processing you can
    /// use a diffent dispatcher (see Extras/BulletMultiThreaded)
    dispatcher =
        std::make_unique<btCollisionDispatcher>(collisionConfiguration.get());

    /// btDbvtBroadphase is a good general purpose broadphase. You can also try
    /// out btAxis3Sweep.
    overlappingPairCache = std::make_unique<btDbvtBroadphase>();

    /// the default constraint solver. For parallel processing you can use a
    /// different solver (see Extras/BulletMultiThreaded)
    solver = std::make_unique<btSequentialImpulseConstraintSolver>();

    dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        dispatcher.get(), overlappingPairCache.get(), solver.get(),
        collisionConfiguration.get());
    debug_line_collecter = std::make_unique<DebugLineCollecter>();
    debug_line_collecter->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
    dynamicsWorld->setDebugDrawer(debug_line_collecter.get());
    //dynamicsWorld->setGravity(btVector3(0, -10, 0));
  }
};

} // namespace physics
