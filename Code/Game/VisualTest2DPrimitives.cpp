#include "Game/VisualTest2DPrimitives.hpp"

#include "Engine/Renderer/DebugRenderSystem.hpp"

constexpr float DISC_MIN_RADIUS = 3.f;
constexpr float DISC_MAX_RADIUS = 6.f;

constexpr float LINE_SEGMENT_MIN_LENGTH = 5.f;
constexpr float LINE_SEGMENT_MAX_LENGTH = 20.f;

constexpr float BOX_MIN_DIMENSION = 5.f;
constexpr float BOX_MAX_DIMENSION = 10.f;

constexpr float CAPSULE_MIN_LENGTH = 5.f;
constexpr float	CAPSULE_MAX_LENGTH = 10.f;
constexpr float CAPSULE_MIN_RADIUS = 3.f;
constexpr float CAPSULE_MAX_RADIUS = 6.f;

constexpr float SECTOR_MIN_RADIUS = 3.f;
constexpr float SECTOR_MAX_RADIUS = 6.f;
constexpr float SECTOR_MIN_APERTURE = 30.f;
constexpr float SECTOR_MAX_APERTURE = 270.f;

VisualTest2DPrimitives::VisualTest2DPrimitives()
{
	g_input->SetCursorMode(false, false);

	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	m_worldCamera.SetOrthoView(Vec2::ZERO, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));

	Randomize();
}

void VisualTest2DPrimitives::Update(float deltaSeconds)
{
	float referencePointSpeed = 40.f;
	
	if (g_input->IsKeyDown('W'))
	{
		m_referencePointPosition += Vec2::NORTH * referencePointSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown('A'))
	{
		m_referencePointPosition += Vec2::WEST * referencePointSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown('S'))
	{
		m_referencePointPosition += Vec2::SOUTH * referencePointSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown('D'))
	{
		m_referencePointPosition += Vec2::EAST * referencePointSpeed * deltaSeconds;
	}
	
	if (g_input->IsKeyDown(KEYCODE_LMB))
	{
		Vec2 normalizedCursorPos = g_input->GetCursorNormalizedPosition();
		AABB2 worldBox(m_worldCamera.GetOrthoBottomLeft(), m_worldCamera.GetOrthoTopRight());
		Vec2 cursorScreenPos = worldBox.GetPointAtUV(normalizedCursorPos);
		m_referencePointPosition = cursorScreenPos;
	}

	DebugAddMessage("F8 to Randomize; LMB/WASD to move reference point", 0.f, Rgba8::CYAN, Rgba8::CYAN);
	DebugAddMessage("Mode [F6/F7 for Prev/Next]: 2D Primitives", 0.f, Rgba8::YELLOW, Rgba8::YELLOW);
}

void VisualTest2DPrimitives::Render() const
{
	g_renderer->BeginCamera(m_worldCamera);

	const Rgba8 SHAPE_COLOR = Rgba8::DODGER_BLUE;
	const Rgba8 SHAPE_HIGHLIGHTED_COLOR = Rgba8::DEEP_SKY_BLUE;
	const Rgba8 NEAREST_POINT_COLOR(200, 200, 0, 255);

	constexpr float LINE_THICKNESS = 0.2f;

	constexpr float referencePointToNearestPointLineThickness = 0.1f;
	const Rgba8 referencePointToNearestPointsLineColor(50, 50, 50, 255);

	constexpr float referencePointRadius = 0.2f;
	constexpr float nearestPointOnShapeRadius = 0.5f;

	std::vector<Vertex_PCU> shapeVertexes;
	// reserve space here if low frame rate

	if (IsPointInsideDisc2D(m_referencePointPosition, m_discCenter, m_discRadius))
	{
		AddVertsForDisc2D(shapeVertexes, m_discCenter, m_discRadius, SHAPE_HIGHLIGHTED_COLOR);
	}
	else
	{
		AddVertsForDisc2D(shapeVertexes, m_discCenter, m_discRadius, SHAPE_COLOR);
	}

	AddVertsForLineSegment2D(shapeVertexes, m_lineSegmentStart, m_lineSegmentEnd, LINE_THICKNESS, SHAPE_COLOR);
	AddVertsForLineSegment2D(shapeVertexes, m_linePointA, m_linePointB, LINE_THICKNESS, SHAPE_COLOR);

	if (IsPointInsideAABB2(m_referencePointPosition, m_axisAlignedBox))
	{
		AddVertsForAABB2(shapeVertexes, m_axisAlignedBox, SHAPE_HIGHLIGHTED_COLOR);
	}
	else
	{
		AddVertsForAABB2(shapeVertexes, m_axisAlignedBox, SHAPE_COLOR);
	}

	if (IsPointInsideOBB2(m_referencePointPosition, m_orientedBox))
	{
		AddVertsForOBB2(shapeVertexes, m_orientedBox, SHAPE_HIGHLIGHTED_COLOR);
	}
	else
	{
		AddVertsForOBB2(shapeVertexes, m_orientedBox, SHAPE_COLOR);
	}

	if (IsPointInsideCapsule2D(m_referencePointPosition, m_capsuleBoneStart, m_capsuleBoneEnd, m_capsuleRadius))
	{
		AddVertsForCapsule2D(shapeVertexes, m_capsuleBoneStart, m_capsuleBoneEnd, m_capsuleRadius, SHAPE_HIGHLIGHTED_COLOR);
	}
	else
	{
		AddVertsForCapsule2D(shapeVertexes, m_capsuleBoneStart, m_capsuleBoneEnd, m_capsuleRadius, SHAPE_COLOR);
	}

	if (IsPointInsideOrientedSector2D(m_referencePointPosition, m_orientedSectorTipPosition, m_orientedSectorOrientationDegrees, m_orientedSectorApertureDegrees, m_orientedSectorRadius))
	{
		AddVertsForOrientedSector2D(shapeVertexes, m_orientedSectorTipPosition, m_orientedSectorOrientationDegrees, m_orientedSectorApertureDegrees, m_orientedSectorRadius, SHAPE_HIGHLIGHTED_COLOR);
	}
	else
	{
		AddVertsForOrientedSector2D(shapeVertexes, m_orientedSectorTipPosition, m_orientedSectorOrientationDegrees, m_orientedSectorApertureDegrees, m_orientedSectorRadius, SHAPE_COLOR);
	}

	if (IsPointInsideDirectedSector2D(m_referencePointPosition, m_directedSectorTipPosition, m_directedSectorForwardNormal, m_directedSectorApertureDegrees, m_directedSectorRadius))
	{
		AddVertsForDirectedSector2D(shapeVertexes, m_directedSectorTipPosition, m_directedSectorForwardNormal, m_directedSectorApertureDegrees, m_directedSectorRadius, SHAPE_HIGHLIGHTED_COLOR);
	}
	else
	{
		AddVertsForDirectedSector2D(shapeVertexes, m_directedSectorTipPosition, m_directedSectorForwardNormal, m_directedSectorApertureDegrees, m_directedSectorRadius, SHAPE_COLOR);
	}

	// Nearest Points
	Vec2 nearestPointOnDisc = GetNearestPointOnDisc2D(m_referencePointPosition, m_discCenter, m_discRadius);
	AddVertsForLineSegment2D(shapeVertexes, m_referencePointPosition, nearestPointOnDisc, referencePointToNearestPointLineThickness, referencePointToNearestPointsLineColor);
	AddVertsForDisc2D(shapeVertexes, nearestPointOnDisc, nearestPointOnShapeRadius, NEAREST_POINT_COLOR);

	Vec2 nearestPointOnLineSegment = GetNearestPointOnLineSegment2D(m_referencePointPosition, m_lineSegmentStart, m_lineSegmentEnd);
	AddVertsForLineSegment2D(shapeVertexes, m_referencePointPosition, nearestPointOnLineSegment, referencePointToNearestPointLineThickness, referencePointToNearestPointsLineColor);
	AddVertsForDisc2D(shapeVertexes, nearestPointOnLineSegment, nearestPointOnShapeRadius, NEAREST_POINT_COLOR);

	Vec2 nearestPointOnInfiniteLine = GetNearestPointOnInfiniteLine2D(m_referencePointPosition, m_linePointA, m_linePointB);
	AddVertsForLineSegment2D(shapeVertexes, m_referencePointPosition, nearestPointOnInfiniteLine, referencePointToNearestPointLineThickness, referencePointToNearestPointsLineColor);
	AddVertsForDisc2D(shapeVertexes, nearestPointOnInfiniteLine, nearestPointOnShapeRadius, NEAREST_POINT_COLOR);

	Vec2 nearestPointOnAABB2 = GetNearestPointOnAABB2(m_referencePointPosition, m_axisAlignedBox);
	AddVertsForLineSegment2D(shapeVertexes, m_referencePointPosition, nearestPointOnAABB2, referencePointToNearestPointLineThickness, referencePointToNearestPointsLineColor);
	AddVertsForDisc2D(shapeVertexes, nearestPointOnAABB2, nearestPointOnShapeRadius, NEAREST_POINT_COLOR);

	Vec2 nearestPointOnOBB2 = GetNearestPointOnOBB2(m_referencePointPosition, m_orientedBox);
	AddVertsForLineSegment2D(shapeVertexes, m_referencePointPosition, nearestPointOnOBB2, referencePointToNearestPointLineThickness, referencePointToNearestPointsLineColor);
	AddVertsForDisc2D(shapeVertexes, nearestPointOnOBB2, nearestPointOnShapeRadius, NEAREST_POINT_COLOR);

	Vec2 nearestPointOnCapsule = GetNearestPointOnCapsule2D(m_referencePointPosition, m_capsuleBoneStart, m_capsuleBoneEnd, m_capsuleRadius);
	AddVertsForLineSegment2D(shapeVertexes, m_referencePointPosition, nearestPointOnCapsule, referencePointToNearestPointLineThickness, referencePointToNearestPointsLineColor);
	AddVertsForDisc2D(shapeVertexes, nearestPointOnCapsule, nearestPointOnShapeRadius, NEAREST_POINT_COLOR);

	Vec2 nearestPointOnOrientedSector = GetNearestPointOnOrientedSector2D(m_referencePointPosition, m_orientedSectorTipPosition, m_orientedSectorOrientationDegrees, m_orientedSectorApertureDegrees, m_orientedSectorRadius);
	AddVertsForLineSegment2D(shapeVertexes, m_referencePointPosition, nearestPointOnOrientedSector, referencePointToNearestPointLineThickness, referencePointToNearestPointsLineColor);
	AddVertsForDisc2D(shapeVertexes, nearestPointOnOrientedSector, nearestPointOnShapeRadius, NEAREST_POINT_COLOR);

	Vec2 nearestPointOnDirectedSector = GetNearestPointOnDirectedSector2D(m_referencePointPosition, m_directedSectorTipPosition, m_directedSectorForwardNormal, m_directedSectorApertureDegrees, m_directedSectorRadius);
	AddVertsForLineSegment2D(shapeVertexes, m_referencePointPosition, nearestPointOnDirectedSector, referencePointToNearestPointLineThickness, referencePointToNearestPointsLineColor);
	AddVertsForDisc2D(shapeVertexes, nearestPointOnDirectedSector, nearestPointOnShapeRadius, NEAREST_POINT_COLOR);

	AddVertsForDisc2D(shapeVertexes, m_referencePointPosition, referencePointRadius, Rgba8(255, 255, 255, 255));

	g_renderer->SetBlendMode(BlendMode::ALPHA);
	g_renderer->SetDepthMode(DepthMode::DISABLED);
	g_renderer->SetModelConstants();
	g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
	g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
	g_renderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
	g_renderer->BindShader(nullptr);
	g_renderer->BindTexture(nullptr);
	g_renderer->DrawVertexArray(shapeVertexes);

	g_renderer->EndCamera(m_worldCamera);
}

void VisualTest2DPrimitives::Randomize()
{
	m_discRadius = g_RNG->RollRandomFloatInRange(DISC_MIN_RADIUS, DISC_MAX_RADIUS);
	m_discCenter = g_RNG->RollRandomVec2InRange(m_discRadius, WORLD_SIZE_X - m_discRadius, m_discRadius, WORLD_SIZE_Y - m_discRadius);
	
	float lineSegmentLength = g_RNG->RollRandomFloatInRange(LINE_SEGMENT_MIN_LENGTH, LINE_SEGMENT_MAX_LENGTH);
	float lineSegmentOrientationDegrees = g_RNG->RollRandomFloatInRange(0.f, 360.f);
	
	m_lineSegmentStart = g_RNG->RollRandomVec2InRange(lineSegmentLength, WORLD_SIZE_X - lineSegmentLength, lineSegmentLength, WORLD_SIZE_Y - lineSegmentLength);
	m_lineSegmentEnd = m_lineSegmentStart + Vec2::MakeFromPolarDegrees(lineSegmentOrientationDegrees, lineSegmentLength);
	
	m_linePointA = g_RNG->RollRandomVec2InRange(0.f, WORLD_SIZE_X, 0.f, WORLD_SIZE_Y);
	m_linePointB = g_RNG->RollRandomVec2InRange(0.f, WORLD_SIZE_X, 0.f, WORLD_SIZE_Y);
	Vec2 lineDirection = (m_linePointB - m_linePointA).GetNormalized();
	m_linePointA -= lineDirection * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);
	m_linePointB += lineDirection * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);
	
	float axisAlignedBoxWidth = g_RNG->RollRandomFloatInRange(BOX_MIN_DIMENSION, BOX_MAX_DIMENSION);
	float axisAlignedBoxHeight = g_RNG->RollRandomFloatInRange(BOX_MIN_DIMENSION, BOX_MAX_DIMENSION);
	Vec2 axisAlignedBoxBL = g_RNG->RollRandomVec2InRange(axisAlignedBoxWidth, WORLD_SIZE_X - axisAlignedBoxWidth, axisAlignedBoxHeight, WORLD_SIZE_Y - axisAlignedBoxHeight);
	Vec2 axisAlignedBoxTR = axisAlignedBoxBL + Vec2(axisAlignedBoxWidth, axisAlignedBoxHeight);
	m_axisAlignedBox = AABB2(axisAlignedBoxBL, axisAlignedBoxTR);
	
	Vec2 orientedBoxHalfDimensions = g_RNG->RollRandomVec2InRange(BOX_MIN_DIMENSION, BOX_MAX_DIMENSION, BOX_MIN_DIMENSION, BOX_MAX_DIMENSION) * 0.5f;
	Vec2 orientedBoxForwardNormal = Vec2(g_RNG->RollRandomFloatInRange(-1.f, 1.f), g_RNG->RollRandomFloatInRange(-1.f, 1.f)).GetNormalized();
	Vec2 orientedBoxCenter = g_RNG->RollRandomVec2InRange(orientedBoxHalfDimensions.x, WORLD_SIZE_X - orientedBoxHalfDimensions.x, orientedBoxHalfDimensions.y, WORLD_SIZE_Y - orientedBoxHalfDimensions.y);
	m_orientedBox = OBB2(orientedBoxCenter, orientedBoxForwardNormal, orientedBoxHalfDimensions);
	
	float capsuleLength = g_RNG->RollRandomFloatInRange(CAPSULE_MIN_LENGTH, CAPSULE_MAX_LENGTH);
	m_capsuleRadius = g_RNG->RollRandomFloatInRange(CAPSULE_MIN_RADIUS, CAPSULE_MAX_RADIUS);
	float capsuleOrientationDegrees = g_RNG->RollRandomFloatInRange(0.f, 360.f);
	m_capsuleBoneStart = g_RNG->RollRandomVec2InRange(capsuleLength, WORLD_SIZE_X - capsuleLength, capsuleLength, WORLD_SIZE_Y - capsuleLength);
	m_capsuleBoneEnd = m_capsuleBoneStart + Vec2::MakeFromPolarDegrees(capsuleOrientationDegrees, capsuleLength);
	
	m_orientedSectorRadius = g_RNG->RollRandomFloatInRange(SECTOR_MIN_RADIUS, SECTOR_MAX_RADIUS);
	m_orientedSectorApertureDegrees = g_RNG->RollRandomFloatInRange(SECTOR_MIN_APERTURE, SECTOR_MAX_APERTURE);
	m_orientedSectorOrientationDegrees = g_RNG->RollRandomFloatInRange(0.f, 360.f);
	m_orientedSectorTipPosition = g_RNG->RollRandomVec2InRange(m_orientedSectorRadius, WORLD_SIZE_X - m_orientedSectorRadius, m_orientedSectorRadius, WORLD_SIZE_Y - m_orientedSectorRadius);
		
	m_directedSectorRadius = g_RNG->RollRandomFloatInRange(SECTOR_MIN_RADIUS, SECTOR_MAX_RADIUS);
	m_directedSectorApertureDegrees = g_RNG->RollRandomFloatInRange(SECTOR_MIN_APERTURE, SECTOR_MAX_APERTURE);
	m_directedSectorForwardNormal = g_RNG->RollRandomVec2InRange(-1.f, 1.f, -1.f, 1.f).GetNormalized();
	m_directedSectorTipPosition = g_RNG->RollRandomVec2InRange(m_directedSectorRadius, WORLD_SIZE_X - m_directedSectorRadius, m_directedSectorRadius, WORLD_SIZE_Y - m_directedSectorRadius);
}
