#include "Game/VisualTest3DGeometry.hpp"

#include "Game/App.hpp"

#include "Engine/Renderer/DebugRenderSystem.hpp"


constexpr int NUM_SHAPES_PER_TYPE = 3;
const AABB3 k_shapesRoom = AABB3(Vec3(-10.f, -10.f, -10.f), Vec3(10.f, 10.f, 10.f));
constexpr float RAYCAST_MAX_DISTANCE = 10.f;

constexpr float MIN_BOX_DIMENSION = 0.5f;
constexpr float MAX_BOX_DIMENSION = 2.f;
constexpr float MIN_RADIUS = 0.5f;
constexpr float MAX_RADIUS = 2.f;
constexpr float MIN_CYLINDER_HEIGHT = 1.f;
constexpr float MAX_CYLINDER_HEIGHT = 5.f;
constexpr int MIN_NUM_SLICES = 4;
constexpr int MAX_NUM_SLICES = 20;
constexpr int MIN_NUM_STACKS = 2;
constexpr int MAX_NUM_STACKS = 10;
constexpr float TEXTURED_CHANCE = 0.5f;

void AddVertsForTexturedPlane3(std::vector<Vertex_PCU>& verts, Plane3 const& plane)
{
	AddVertsForQuad3D(verts, Vec3(-50.f, 50.f, 0.f), Vec3(-50.f, -50.f, 0.f), Vec3(50.f, -50.f, 0.f), Vec3(50.f, 50.f, 0.f), Rgba8::WHITE);
	Vec3 planeCenter = plane.GetCenter();
	Vec3 planeIBasis;
	Vec3 planeJBasis;
	if (CrossProduct3D(Vec3::SKYWARD, plane.m_normal) == Vec3::ZERO)
	{
		planeJBasis = Vec3::NORTH;
	}
	else
	{
		planeJBasis = CrossProduct3D(Vec3::SKYWARD, plane.m_normal).GetNormalized();
	}

	planeIBasis = CrossProduct3D(planeJBasis, plane.m_normal).GetNormalized();

	Mat44 planeTransformMatrix = Mat44(planeIBasis, planeJBasis, plane.m_normal, planeCenter);
	TransformVertexArray3D(verts, planeTransformMatrix);
}

TestShape3D::TestShape3D(Shape3DData data)
	: m_data(data)
{
	m_color = m_data.m_color;

	switch (m_data.m_type)
	{
		case ShapeType3D::AABB3:
		{
			AddVertsForAABB3(m_vertexes, AABB3(m_data.m_mins, m_data.m_maxs), m_data.m_color);
			break;
		}
		case ShapeType3D::OBB3:
		{
			AddVertsForOBB3(m_vertexes, OBB3(m_data.m_center, m_data.m_halfDimensions, m_data.m_iBasis, m_data.m_jBasis), m_data.m_color);
			break;
		}
		case ShapeType3D::SPHERE:
		{
			AddVertsForSphere3D(m_vertexes, m_data.m_center, m_data.m_radius, m_data.m_color, AABB2::ZERO_TO_ONE, m_data.m_numStacks);
			break;
		}
		case ShapeType3D::CYLINDER:
		{
			AddVertsForCylinder3D(m_vertexes, m_data.m_startCenter, m_data.m_endCenter, m_data.m_radius, m_data.m_color, AABB2::ZERO_TO_ONE, m_data.m_numSlices);
			break;
		}
		case ShapeType3D::PLANE3:
		{
			if (!m_data.m_texture)
			{
				AddVertsForWireframePlane3(m_vertexes, Plane3(m_data.m_normal, m_data.m_distanceFromOriginAlongNormal));
			}
			else
			{
				AddVertsForTexturedPlane3(m_vertexes, Plane3(m_data.m_normal, m_data.m_distanceFromOriginAlongNormal));
			}
		}
	}
}

Vec3 const TestShape3D::GetNearestPoint(Vec3 const& referencePosition) const
{
	switch (m_data.m_type)
	{
		case ShapeType3D::AABB3:		return GetNearestPointOnAABB3(referencePosition, AABB3(m_translation + m_data.m_mins, m_translation + m_data.m_maxs));
		case ShapeType3D::OBB3:			return GetNearestPointOnOBB3(referencePosition, OBB3(m_translation + m_data.m_center, m_data.m_halfDimensions, m_data.m_iBasis, m_data.m_jBasis));
		case ShapeType3D::SPHERE:		return GetNearestPointOnSphere3D(referencePosition, m_translation + m_data.m_center, m_data.m_radius);
		case ShapeType3D::CYLINDER:		return GetNearestPointOnCylinder3D(referencePosition, m_translation + m_data.m_startCenter, m_translation + m_data.m_endCenter, m_data.m_radius);
		case ShapeType3D::PLANE3:		return GetNearestPointOnPlane3(referencePosition, Plane3(m_data.m_normal, GetProjectedLength3D(m_translation, m_data.m_normal) + m_data.m_distanceFromOriginAlongNormal));
	}

	return Vec3::ZERO;
}

RaycastResult3D TestShape3D::Raycast(Vec3 const& rayStart, Vec3 const& rayFwdNormal, float rayMaxDistance) const
{
	switch (m_data.m_type)
	{
		case ShapeType3D::AABB3:
		{
			 return RaycastVsAABB3(rayStart, rayFwdNormal, rayMaxDistance, AABB3(m_translation + m_data.m_mins, m_translation + m_data.m_maxs));
		}
		case ShapeType3D::OBB3:
		{
			return RaycastVsOBB3(rayStart, rayFwdNormal, rayMaxDistance, OBB3(m_translation + m_data.m_center, m_data.m_halfDimensions, m_data.m_iBasis, m_data.m_jBasis));
		}
		case ShapeType3D::SPHERE:
		{
			return RaycastVsSphere(rayStart, rayFwdNormal, rayMaxDistance, m_translation + m_data.m_center, m_data.m_radius);
		}
		case ShapeType3D::CYLINDER:
		{
			return RaycastVsCylinder3D(rayStart, rayFwdNormal, rayMaxDistance, m_translation + m_data.m_startCenter, m_translation + m_data.m_endCenter, m_data.m_radius);
		}
		case ShapeType3D::PLANE3:
		{
			return RaycastVsPlane3(rayStart, rayFwdNormal, rayMaxDistance, Plane3(m_data.m_normal, GetProjectedLength3D(m_translation, m_data.m_normal) + m_data.m_distanceFromOriginAlongNormal));
		}
	}

	return RaycastResult3D();
}

bool TestShape3D::DoesOverlapShape(TestShape3D otherShape) const
{
	switch (otherShape.m_data.m_type)
	{
		case ShapeType3D::AABB3:			return DoesOverlapAABB3(AABB3(otherShape.m_translation + otherShape.m_data.m_mins, otherShape.m_translation + otherShape.m_data.m_maxs));
		case ShapeType3D::OBB3:				return DoesOverlapOBB3(OBB3(otherShape.m_translation + otherShape.m_data.m_center, otherShape.m_data.m_halfDimensions, otherShape.m_data.m_iBasis, otherShape.m_data.m_jBasis, otherShape.m_data.m_kBasis));
		case ShapeType3D::CYLINDER:			return DoesOverlapZCylinder(otherShape.m_translation + otherShape.m_data.m_startCenter, otherShape.m_translation + otherShape.m_data.m_endCenter, otherShape.m_data.m_radius);
		case ShapeType3D::SPHERE:			return DoesOverlapSphere(otherShape.m_translation + otherShape.m_data.m_center, otherShape.m_data.m_radius);
		case ShapeType3D::PLANE3:			return DoesOverlapPlane3(Plane3(otherShape.m_data.m_normal, GetProjectedLength3D(otherShape.m_translation, otherShape.m_data.m_normal) + otherShape.m_data.m_distanceFromOriginAlongNormal));
	}

	return false;
}

bool TestShape3D::DoesOverlapAABB3(AABB3 box) const
{
	switch (m_data.m_type)
	{
		case ShapeType3D::AABB3:		return DoAABB3Overlap(AABB3(m_translation + m_data.m_mins, m_translation + m_data.m_maxs), box);
		case ShapeType3D::OBB3:			return false;
		case ShapeType3D::SPHERE:		return DoSphereAndAABB3Overlap(m_translation + m_data.m_center, m_data.m_radius, box);
		case ShapeType3D::CYLINDER:		return DoZCylinderAndAABB3Overlap(m_translation + m_data.m_startCenter, m_translation + m_data.m_endCenter, m_data.m_radius, box);
		case ShapeType3D::PLANE3:		return DoPlane3AndAABB3Overlap(Plane3(m_data.m_normal, GetProjectedLength3D(m_translation, m_data.m_normal) + m_data.m_distanceFromOriginAlongNormal), box);
	}

	return false;
}

bool TestShape3D::DoesOverlapOBB3(OBB3 box) const
{
	switch (m_data.m_type)
	{
		case ShapeType3D::AABB3:		return false;
		case ShapeType3D::OBB3:			return false;
		case ShapeType3D::SPHERE:		return DoSphereAndOBB3Overlap(m_translation + m_data.m_center, m_data.m_radius, box);
		case ShapeType3D::CYLINDER:		return false;
		case ShapeType3D::PLANE3:		return DoPlane3AndOBB3Overlap(Plane3(m_data.m_normal, GetProjectedLength3D(m_translation, m_data.m_normal) + m_data.m_distanceFromOriginAlongNormal), box);
	}

	return false;
}

bool TestShape3D::DoesOverlapZCylinder(Vec3 const& baseCenter, Vec3 const& topCenter, float radius) const
{
	switch (m_data.m_type)
	{
		case ShapeType3D::AABB3:		return DoAABB3AndZCylinderOverlap(AABB3(m_translation + m_data.m_mins, m_translation + m_data.m_maxs), baseCenter, topCenter, radius);
		case ShapeType3D::OBB3:			return false;
		case ShapeType3D::SPHERE:		return DoSphereAndCylinderOverlap(m_translation + m_data.m_center, m_data.m_radius, baseCenter, topCenter, radius);
		case ShapeType3D::CYLINDER:		return DoZCylindersOverlap(m_translation + m_data.m_startCenter, m_translation + m_data.m_endCenter, m_data.m_radius, baseCenter, topCenter, radius);
		case ShapeType3D::PLANE3:		return false;
	}

	return false;
}

bool TestShape3D::DoesOverlapSphere(Vec3 const& sphereCenter, float sphereRadius) const
{
	switch (m_data.m_type)
	{
		case ShapeType3D::AABB3:		return DoAABB3AndSphereOverlap(AABB3(m_translation + m_data.m_mins, m_translation + m_data.m_maxs), sphereCenter, sphereRadius);
		case ShapeType3D::OBB3:			return DoOBB3AndSphereOverlap(OBB3(m_translation + m_data.m_center, m_data.m_halfDimensions, m_data.m_iBasis, m_data.m_jBasis, m_data.m_kBasis), sphereCenter, sphereRadius);
		case ShapeType3D::SPHERE:		return DoSpheresOverlap(m_translation + m_data.m_center, m_data.m_radius, sphereCenter, sphereRadius);
		case ShapeType3D::CYLINDER:		return DoCylinderAndSphereOverlap(m_translation + m_data.m_startCenter, m_translation + m_data.m_endCenter, m_data.m_radius, sphereCenter, sphereRadius);
		case ShapeType3D::PLANE3:		return DoSphereAndPlane3Overlap(sphereCenter, sphereRadius, Plane3(m_data.m_normal, GetProjectedLength3D(m_translation, m_data.m_normal) + m_data.m_distanceFromOriginAlongNormal));
	}

	return false;
}

bool TestShape3D::DoesOverlapPlane3(Plane3 plane) const
{
	switch (m_data.m_type)
	{
		case ShapeType3D::AABB3:		return DoAABB3AndPlane3Overlap(AABB3(m_translation + m_data.m_mins, m_translation + m_data.m_maxs), plane);
		case ShapeType3D::OBB3:			return DoOBB3AndPlane3Overlap(OBB3(m_translation + m_data.m_center, m_data.m_halfDimensions, m_data.m_iBasis, m_data.m_jBasis, m_data.m_kBasis), plane);
		case ShapeType3D::SPHERE:		return DoSphereAndPlane3Overlap(m_translation + m_data.m_center, m_data.m_radius, plane);
		case ShapeType3D::CYLINDER:		return false;
		case ShapeType3D::PLANE3:		return false;
	}

	return false;
}

void TestShape3D::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (m_isSelected)
	{
		m_color = Rgba8::RED;
	}
	else if (m_isImpactShape)
	{
		m_color = Rgba8::BLUE;
	}
	else
	{
		m_color = Rgba8::WHITE;
	}

	if (m_isBlinking)
	{
		float timeInMode = Clock::GetSystemClock().GetTotalSeconds();
		m_color = Interpolate(m_color, Rgba8::BLACK, RangeMapClamped(sinf(4.f * timeInMode), -1.f, 1.f, 0.f, 0.5f));
	}

	m_isImpactShape = false;
	m_isBlinking = false;
}

void TestShape3D::Render() const
{
	g_renderer->SetBlendMode(BlendMode::OPAQUE);
	g_renderer->SetDepthMode(DepthMode::ENABLED);
	g_renderer->SetModelConstants(Mat44::CreateTranslation3D(m_translation), m_color);
	g_renderer->SetRasterizerCullMode(m_data.m_type != ShapeType3D::PLANE3 && m_data.m_rasterizerFillMode == RasterizerFillMode::SOLID ? RasterizerCullMode::CULL_BACK : RasterizerCullMode::CULL_NONE);
	g_renderer->SetRasterizerFillMode(m_data.m_type == ShapeType3D::PLANE3 ? RasterizerFillMode::SOLID : m_data.m_rasterizerFillMode);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->BindShader(nullptr);
	g_renderer->BindTexture(m_data.m_texture);
	g_renderer->DrawVertexArray(m_vertexes);
}

VisualTest3DGeometry::VisualTest3DGeometry()
{
	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	m_worldCamera.SetPerspectiveView(g_window->GetAspect(), 60.f, 0.1f, 100.f);
	m_worldCamera.SetRenderBasis(Vec3::SKYWARD, Vec3::WEST, Vec3::NORTH);
	m_worldCamera.Translate3D(Vec3::WEST * 3.f);

	Randomize();

	DebugAddWorldPoint(Vec3::ZERO, 0.04f, -1.f, Rgba8::WHITE);
	DebugAddWorldArrow(Vec3::ZERO, Vec3::EAST, 0.02f, -1.f, Rgba8::RED);
	DebugAddWorldArrow(Vec3::ZERO, Vec3::NORTH, 0.02f, -1.f, Rgba8::GREEN);
	DebugAddWorldArrow(Vec3::ZERO, Vec3::SKYWARD, 0.02f, -1.f, Rgba8::BLUE);
}

void VisualTest3DGeometry::Update(float deltaSeconds)
{
	float movementSpeed = g_input->IsShiftHeld() ? 10.f : 2.f;
	constexpr float TURN_RATE_PER_MOUSE_CLIENT_DELTA = 0.075f;

	EulerAngles cameraOrientation = m_worldCamera.GetOrientation();
	Vec3 cameraFwd;
	Vec3 cameraLeft;
	Vec3 cameraUp;
	cameraOrientation.GetAsVectors_iFwd_jLeft_kUp(cameraFwd, cameraLeft, cameraUp);

	for (int shapeIndex = 0; shapeIndex < (int)m_shapes.size(); shapeIndex++)
	{
		m_shapes[shapeIndex].Update(deltaSeconds);
	}
	TestShapeRaycastResult3D raycastResult = RaycastVsAll(m_referencePosition, m_referenceDirection, RAYCAST_MAX_DISTANCE);
	if (raycastResult.m_didImpact)
	{
		m_impactShape = raycastResult.m_impactShape;
		raycastResult.m_impactShape->m_isImpactShape = true;
	}
	for (int shape1Index = 0; shape1Index < (int)m_shapes.size() - 1; shape1Index++)
	{
		for (int shape2Index = shape1Index + 1; shape2Index < (int)m_shapes.size(); shape2Index++)
		{
			if (m_shapes[shape1Index].DoesOverlapShape(m_shapes[shape2Index]))
			{
				m_shapes[shape1Index].m_isBlinking = true;
				m_shapes[shape2Index].m_isBlinking = true;
			}
		}
	}

	if (g_input->IsKeyDown('W'))
	{
		m_worldCamera.Translate3D(Vec3(cameraFwd.x, cameraFwd.y, 0.f).GetNormalized() * movementSpeed * deltaSeconds);
		if (m_selectedShape)
		{
			m_selectedShape->m_translation += Vec3(cameraFwd.x, cameraFwd.y, 0.f).GetNormalized() * movementSpeed * deltaSeconds;
		}
	}
	if (g_input->IsKeyDown('S'))
	{
		m_worldCamera.Translate3D(-Vec3(cameraFwd.x, cameraFwd.y, 0.f).GetNormalized() * movementSpeed * deltaSeconds);
		if (m_selectedShape)
		{
			m_selectedShape->m_translation += -Vec3(cameraFwd.x, cameraFwd.y, 0.f).GetNormalized() * movementSpeed * deltaSeconds;
		}
	}
	if (g_input->IsKeyDown('A'))
	{
		m_worldCamera.Translate3D(Vec3(cameraLeft.x, cameraLeft.y, 0.f).GetNormalized() * movementSpeed * deltaSeconds);
		if (m_selectedShape)
		{
			m_selectedShape->m_translation += Vec3(cameraLeft.x, cameraLeft.y, 0.f).GetNormalized() * movementSpeed * deltaSeconds;
		}
	}
	if (g_input->IsKeyDown('D'))
	{
		m_worldCamera.Translate3D(-Vec3(cameraLeft.x, cameraLeft.y, 0.f).GetNormalized() * movementSpeed * deltaSeconds);
		if (m_selectedShape)
		{
			m_selectedShape->m_translation += -Vec3(cameraLeft.x, cameraLeft.y, 0.f).GetNormalized() * movementSpeed * deltaSeconds;
		}
	}
	if (g_input->IsKeyDown('Q'))
	{
		m_worldCamera.Translate3D(Vec3::GROUNDWARD * movementSpeed * deltaSeconds);
		if (m_selectedShape)
		{
			m_selectedShape->m_translation += Vec3::GROUNDWARD * movementSpeed * deltaSeconds;
		}
	}
	if (g_input->IsKeyDown('E'))
	{
		m_worldCamera.Translate3D(Vec3::SKYWARD * movementSpeed * deltaSeconds);
		if (m_selectedShape)
		{
			m_selectedShape->m_translation += Vec3::SKYWARD * movementSpeed * deltaSeconds;
		}
	}

	if (g_input->WasKeyJustPressed(KEYCODE_SPACE))
	{
		m_refLocked = !m_refLocked;
	}
	if (g_input->WasKeyJustPressed(KEYCODE_LMB))
	{
		if (m_selectedShape)
		{
			m_selectedShape->m_isSelected = false;
			m_selectedShape = nullptr;
		}
		else if (m_impactShape)
		{
			m_impactShape->m_isSelected = true;
			m_selectedShape = m_impactShape;
		}
	}

	float yawRotation = TURN_RATE_PER_MOUSE_CLIENT_DELTA * (float)g_input->GetCursorClientDelta().x;
	float pitchRotation = -TURN_RATE_PER_MOUSE_CLIENT_DELTA * (float)g_input->GetCursorClientDelta().y;
	m_worldCamera.Rotate3D(EulerAngles(yawRotation, pitchRotation, 0.f));
	m_worldCamera.ClampPitch(89.9f);

	if (!g_window->HasFocus())
	{
		g_input->SetCursorMode(false, false);
	}
	else
	{
		g_input->SetCursorMode(true, true);
	}

	cameraOrientation.GetAsVectors_iFwd_jLeft_kUp(cameraFwd, cameraLeft, cameraUp);

	DebugAddWorldPoint(m_worldCamera.GetPosition() + cameraFwd * 0.2f, 0.0001f, 0.f, Rgba8::WHITE);
	DebugAddWorldArrow(m_worldCamera.GetPosition() + cameraFwd * 0.2f, m_worldCamera.GetPosition() + cameraFwd * 0.2f + Vec3::EAST * 0.01f, 0.0005f, 0.f, Rgba8::RED);
	DebugAddWorldArrow(m_worldCamera.GetPosition() + cameraFwd * 0.2f, m_worldCamera.GetPosition() + cameraFwd * 0.2f + Vec3::NORTH * 0.01f, 0.0005f, 0.f, Rgba8::GREEN);
	DebugAddWorldArrow(m_worldCamera.GetPosition() + cameraFwd * 0.2f, m_worldCamera.GetPosition() + cameraFwd * 0.2f + Vec3::SKYWARD * 0.01f, 0.0005f, 0.f, Rgba8::BLUE);
	
	if (m_impactShape)
	{
		DebugAddWorldPoint(raycastResult.m_impactPosition, 0.03f, 0.f);
		DebugAddWorldArrow(raycastResult.m_impactPosition, raycastResult.m_impactPosition + raycastResult.m_impactNormal, 0.02f, 0.f, Rgba8::YELLOW, Rgba8::YELLOW);

		if (m_refLocked)
		{
			DebugAddWorldArrow(m_referencePosition, raycastResult.m_impactPosition, 0.021f, 0.f, Rgba8::RED, Rgba8::RED);
			DebugAddWorldArrow(m_referencePosition, m_referencePosition + m_referenceDirection * RAYCAST_MAX_DISTANCE, 0.02f, 0.f, Rgba8::GRAY, Rgba8::GRAY);
		}
	}
	else if (m_refLocked)
	{
		DebugAddWorldArrow(m_referencePosition, m_referencePosition + m_referenceDirection * RAYCAST_MAX_DISTANCE, 0.02f, 0.f, Rgba8::GREEN, Rgba8::GREEN);
	}


	if (!m_refLocked)
	{
		m_referencePosition = m_worldCamera.GetPosition();
		m_referenceDirection = cameraFwd;
	}


	std::string controlsText = "F8 to Randomize; WASD/QE to move in XY plane or skyward/groundward; Shift (Hold) to sprint; ";
	if (m_refLocked)
	{
		controlsText += "Spacebar to unlock reference; ";
	}
	else
	{
		controlsText += "Spacebar to lock reference; ";
	}
	if (m_impactShape && !m_selectedShape)
	{
		controlsText += "LMB to grab shape; ";
	}
	else if (m_selectedShape)
	{
		controlsText += "LMB to release shape; ";
	}

	DebugAddMessage(Stringf("%s", controlsText.c_str()), 0.f, Rgba8::CYAN, Rgba8::CYAN);
	DebugAddMessage("Mode [F6/F7 for Prev/Next]: 3D Geometry", 0.f, Rgba8::YELLOW, Rgba8::YELLOW);

	m_impactShape = nullptr;
}

void VisualTest3DGeometry::Render() const
{
	constexpr float POINT_RADIUS = 0.03f;

	g_renderer->BeginCamera(m_worldCamera);
	for (int shapeIndex = 0; shapeIndex < (int)m_shapes.size(); shapeIndex++)
	{
		TestShape3D const& shape = m_shapes[shapeIndex];

		shape.Render();

		Vec3 nearestPointOnShape = shape.GetNearestPoint(m_referencePosition);
		DebugAddWorldPoint(nearestPointOnShape, POINT_RADIUS, 0.f, Rgba8::ORANGE, Rgba8::ORANGE);
	}

	g_renderer->EndCamera(m_worldCamera);
}

void VisualTest3DGeometry::Randomize()
{
	m_shapes.clear();

	for (int shapeType = 0; shapeType < (int)ShapeType3D::COUNT; shapeType++)
	{
		for (int shapeIndex = 0; shapeIndex < NUM_SHAPES_PER_TYPE; shapeIndex++)
		{
			Shape3DData shapeData;
			shapeData.m_color = Rgba8::WHITE;
			shapeData.m_rasterizerFillMode = RasterizerFillMode::WIREFRAME;
			shapeData.m_type = ShapeType3D(shapeType);
			shapeData.m_radius = g_RNG->RollRandomFloatInRange(MIN_RADIUS, MAX_RADIUS);
			shapeData.m_mins = g_RNG->RollRandomVec3InAABB3(k_shapesRoom);
			float boxXDim = g_RNG->RollRandomFloatInRange(MIN_BOX_DIMENSION, MAX_BOX_DIMENSION);
			float boxYDim = g_RNG->RollRandomFloatInRange(MIN_BOX_DIMENSION, MAX_BOX_DIMENSION);
			float boxZDim = g_RNG->RollRandomFloatInRange(MIN_BOX_DIMENSION, MAX_BOX_DIMENSION);
			shapeData.m_maxs = shapeData.m_mins + Vec3(boxXDim, boxYDim, boxZDim);
			shapeData.m_center = g_RNG->RollRandomVec3InAABB3(k_shapesRoom);
			shapeData.m_startCenter = g_RNG->RollRandomVec3InAABB3(k_shapesRoom);
			float cylinderHeight = g_RNG->RollRandomFloatInRange(MIN_CYLINDER_HEIGHT, MAX_CYLINDER_HEIGHT);
			shapeData.m_endCenter = shapeData.m_startCenter + Vec3(0.f, 0.f, cylinderHeight);
			shapeData.m_halfDimensions.x = g_RNG->RollRandomFloatInRange(MIN_BOX_DIMENSION, MAX_BOX_DIMENSION);
			shapeData.m_halfDimensions.y = g_RNG->RollRandomFloatInRange(MIN_BOX_DIMENSION, MAX_BOX_DIMENSION);
			shapeData.m_halfDimensions.z = g_RNG->RollRandomFloatInRange(MIN_BOX_DIMENSION, MAX_BOX_DIMENSION);
			shapeData.m_iBasis = g_RNG->RollRandomVec3InRadius(Vec3::ZERO, 1.f);
			if (CrossProduct3D(Vec3::SKYWARD, shapeData.m_iBasis) == Vec3::ZERO)
			{
				shapeData.m_jBasis = Vec3::NORTH;
			}
			else
			{
				shapeData.m_jBasis = CrossProduct3D(Vec3::SKYWARD, shapeData.m_iBasis).GetNormalized();
			}
			shapeData.m_kBasis = CrossProduct3D(shapeData.m_iBasis, shapeData.m_jBasis).GetNormalized();
			shapeData.m_numStacks = g_RNG->RollRandomIntInRange(MIN_NUM_STACKS, MAX_NUM_STACKS);
			shapeData.m_numSlices = g_RNG->RollRandomIntInRange(MIN_NUM_SLICES, MAX_NUM_SLICES);
			if (g_RNG->RollRandomChance(TEXTURED_CHANCE))
			{
				shapeData.m_texture = g_app->m_testTexture;
				shapeData.m_rasterizerFillMode = RasterizerFillMode::SOLID;
			}
			shapeData.m_normal = g_RNG->RollRandomVec3InRadius(Vec3::ZERO, 1.f);
			shapeData.m_distanceFromOriginAlongNormal = g_RNG->RollRandomFloatInRange(k_shapesRoom.m_mins.x, k_shapesRoom.m_maxs.x);

			TestShape3D shape = TestShape3D(shapeData);
			m_shapes.push_back(shape);

			if (shapeData.m_type == ShapeType3D::PLANE3)
			{
				break;
			}
		}
	}
}

TestShapeRaycastResult3D VisualTest3DGeometry::RaycastVsAll(Vec3 const& startPos, Vec3 const& fwdNormal, float maxDistance)
{
	TestShapeRaycastResult3D result;
	result.m_rayStartPosition = startPos;
	result.m_rayForwardNormal = fwdNormal;
	result.m_rayMaxLength = maxDistance;
	result.m_impactDistance = FLT_MAX;

	for (int shapeIndex = 0; shapeIndex < (int)m_shapes.size(); shapeIndex++)
	{
		TestShape3D& shape = m_shapes[shapeIndex];
		RaycastResult3D raycastResult = shape.Raycast(startPos, fwdNormal, maxDistance);

		if (raycastResult.m_didImpact && raycastResult.m_impactDistance < result.m_impactDistance)
		{
			result.m_didImpact = true;
			result.m_impactPosition = raycastResult.m_impactPosition;
			result.m_impactDistance = raycastResult.m_impactDistance;
			result.m_impactNormal = raycastResult.m_impactNormal;
			result.m_impactShape = &shape;
		}
	}

	return result;
}
