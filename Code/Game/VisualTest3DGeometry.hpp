#pragma once

#include "Game/Game.hpp"

#include "Engine/Math/Plane3.hpp"
#include "Engine/Math/RaycastUtils.hpp"


enum class ShapeType3D
{
	INVALID = -1,

	CYLINDER,
	SPHERE,
	AABB3,
	OBB3,
	PLANE3,

	COUNT
};

struct Shape3DData
{
	ShapeType3D m_type = ShapeType3D::INVALID;
	RasterizerFillMode m_rasterizerFillMode = RasterizerFillMode::SOLID;
	Texture* m_texture = nullptr;
	Rgba8 m_color = Rgba8::WHITE;

	float m_radius = 0.f;
	Vec3 m_mins = Vec3::ZERO;
	Vec3 m_maxs = Vec3::ZERO;
	Vec3 m_center = Vec3::ZERO;
	Vec3 m_startCenter = Vec3::ZERO;
	Vec3 m_endCenter = Vec3::ZERO;
	Vec3 m_halfDimensions = Vec3::ZERO;
	Vec3 m_iBasis = Vec3::ZERO;
	Vec3 m_jBasis = Vec3::ZERO;
	Vec3 m_kBasis = Vec3::ZERO;
	int m_numStacks = 8;
	int m_numSlices = 16;
	Vec3 m_normal = Vec3::ZERO;
	float m_distanceFromOriginAlongNormal = 0.f;
};

class TestShape3D
{
public:
	std::vector<Vertex_PCU> m_vertexes;
	Shape3DData m_data;
	bool m_isBlinking = false;
	Rgba8 m_color = Rgba8::WHITE;
	bool m_isImpactShape = false;
	bool m_isSelected = false;
	Vec3 m_translation = Vec3::ZERO;

public:
	~TestShape3D() = default;
	TestShape3D(Shape3DData data);
	Vec3 const GetNearestPoint(Vec3 const& referencePosition) const;
	RaycastResult3D Raycast(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayMaxDistance) const;
	bool DoesOverlapShape(TestShape3D otherShape) const;
	bool DoesOverlapAABB3(AABB3 box) const;
	bool DoesOverlapOBB3(OBB3 box) const;
	bool DoesOverlapZCylinder(Vec3 const& baseCenter, Vec3 const& topCenter, float radius) const;
	bool DoesOverlapSphere(Vec3 const& sphereCenter, float sphereRadius) const;
	bool DoesOverlapPlane3(Plane3 plane) const;
	void Update(float deltaSeconds);
	void Render() const;
};

struct TestShapeRaycastResult3D : public RaycastResult3D
{
public:
	TestShape3D* m_impactShape = nullptr;
};

class VisualTest3DGeometry : public Game
{
public:
	~VisualTest3DGeometry() = default;
	VisualTest3DGeometry();
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void Randomize() override;
	TestShapeRaycastResult3D RaycastVsAll(Vec3 const& startPos, Vec3 const& fwdNormal, float maxDistance);
public:
	std::vector<TestShape3D> m_shapes;
	Vec3 m_referencePosition = Vec3::ZERO;
	Vec3 m_referenceDirection = Vec3::ZERO;
	bool m_refLocked = false;
	TestShape3D* m_selectedShape = nullptr;
	TestShape3D* m_impactShape = nullptr;
};

void AddVertsForTexturedPlane3(std::vector<Vertex_PCU>& verts, Plane3 const& plane);
