#include "Game/VisualTestRaycastVsLineSegments.hpp"

#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"

static constexpr int NUM_LINE_SEGMENTS = 10;
constexpr float LINE_SEGMENT_MIN_LENGTH = 5.f;
constexpr float LINE_SEGMENT_MAX_LENGTH = 20.f;

VisualTestRaycastVsLineSegments::VisualTestRaycastVsLineSegments()
{
	g_input->SetCursorMode(false, false);

	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	m_worldCamera.SetOrthoView(Vec2::ZERO, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));

	Randomize();
}

void VisualTestRaycastVsLineSegments::Update(float deltaSeconds)
{
	float pointsMovementSpeed = 40.f;

	if (g_input->IsKeyDown('W'))
	{
		m_raycastStartPosition += Vec2::NORTH * pointsMovementSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown('A'))
	{
		m_raycastStartPosition += Vec2::WEST * pointsMovementSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown('S'))
	{
		m_raycastStartPosition += Vec2::SOUTH * pointsMovementSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown('D'))
	{
		m_raycastStartPosition += Vec2::EAST * pointsMovementSpeed * deltaSeconds;
	}

	if (g_input->IsKeyDown('I'))
	{
		m_raycastEndPosition += Vec2::NORTH * pointsMovementSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown('J'))
	{
		m_raycastEndPosition += Vec2::WEST * pointsMovementSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown('K'))
	{
		m_raycastEndPosition += Vec2::SOUTH * pointsMovementSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown('L'))
	{
		m_raycastEndPosition += Vec2::EAST * pointsMovementSpeed * deltaSeconds;
	}

	if (g_input->IsKeyDown(KEYCODE_UPARROW))
	{
		m_raycastStartPosition += Vec2::NORTH * pointsMovementSpeed * deltaSeconds;
		m_raycastEndPosition += Vec2::NORTH * pointsMovementSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown(KEYCODE_LEFTARROW))
	{
		m_raycastStartPosition += Vec2::WEST * pointsMovementSpeed * deltaSeconds;
		m_raycastEndPosition += Vec2::WEST * pointsMovementSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown(KEYCODE_DOWNARROW))
	{
		m_raycastStartPosition += Vec2::SOUTH * pointsMovementSpeed * deltaSeconds;
		m_raycastEndPosition += Vec2::SOUTH * pointsMovementSpeed * deltaSeconds;
	}
	if (g_input->IsKeyDown(KEYCODE_RIGHTARROW))
	{
		m_raycastStartPosition += Vec2::EAST * pointsMovementSpeed * deltaSeconds;
		m_raycastEndPosition += Vec2::EAST * pointsMovementSpeed * deltaSeconds;
	}

	if (g_input->IsKeyDown(KEYCODE_LMB))
	{
		DebuggerPrintf("KEYCODE_LMB DOWN\n");
		Vec2 normalizedCursorPos = g_input->GetCursorNormalizedPosition();
		AABB2 screenBox(m_worldCamera.GetOrthoBottomLeft(), m_worldCamera.GetOrthoTopRight());
		Vec2 cursorScreenPos = screenBox.GetPointAtUV(normalizedCursorPos);
		m_raycastStartPosition = cursorScreenPos;
	}
	if (g_input->IsKeyDown(KEYCODE_RMB))
	{
		Vec2 normalizedCursorPos = g_input->GetCursorNormalizedPosition();
		AABB2 screenBox(m_worldCamera.GetOrthoBottomLeft(), m_worldCamera.GetOrthoTopRight());
		Vec2 cursorScreenPos = screenBox.GetPointAtUV(normalizedCursorPos);
		m_raycastEndPosition = cursorScreenPos;
	}

	DebugAddMessage("F8 to randomize; LMB/RMB set ray start/end; WASD to move start, IJKL to move end, arrow keys to move ray", 0.f, Rgba8::CYAN, Rgba8::CYAN);
	DebugAddMessage("Mode [F6/F7 for Prev/Next]: Raycast vs Line Segments", 0.f, Rgba8::YELLOW, Rgba8::YELLOW);
}

void VisualTestRaycastVsLineSegments::Render() const
{
	g_renderer->BeginCamera(m_worldCamera);

	constexpr float LINE_THICKNESS = 0.2f;

	std::vector<Vertex_PCU> raycastVsLineSegmentVerts;
	for (int lineSegmentIndex = 0; lineSegmentIndex < NUM_LINE_SEGMENTS; lineSegmentIndex++)
	{
		AddVertsForLineSegment2D(raycastVsLineSegmentVerts, m_lineSegments[lineSegmentIndex].m_start, m_lineSegments[lineSegmentIndex].m_end, LINE_THICKNESS, Rgba8::NAVY);
	}

	RaycastResult2D raycastResult;

	Vec2 raycastDirection = (m_raycastEndPosition - m_raycastStartPosition).GetNormalized();
	float raycastMaxLength = GetDistance2D(m_raycastStartPosition, m_raycastEndPosition);

	LineSegment2D closestImpactLineSegment;
	RaycastResult2D closestRaycast;
	closestRaycast.m_impactDistance = 99999.f;

	for (int lineSegmentIndex = 0; lineSegmentIndex < NUM_LINE_SEGMENTS; lineSegmentIndex++)
	{
		LineSegment2D const& lineSegment = m_lineSegments[lineSegmentIndex];

		raycastResult = RaycastVsLineSegment2D(m_raycastStartPosition, raycastDirection, raycastMaxLength, lineSegment.m_start, lineSegment.m_end);

		if (raycastResult.m_didImpact)
		{
			if (raycastResult.m_impactDistance < closestRaycast.m_impactDistance)
			{
				closestRaycast = raycastResult;
				closestImpactLineSegment = lineSegment;
			}
		}
	}

	if (closestRaycast.m_didImpact)
	{
		AddVertsForLineSegment2D(raycastVsLineSegmentVerts, closestImpactLineSegment.m_start, closestImpactLineSegment.m_end, LINE_THICKNESS, Rgba8::DODGER_BLUE);
		AddVertsForArrow2D(raycastVsLineSegmentVerts, m_raycastStartPosition, m_raycastEndPosition, 2.f, 0.1f, Rgba8::GRAY);
		AddVertsForArrow2D(raycastVsLineSegmentVerts, m_raycastStartPosition, closestRaycast.m_impactPosition, 2.f, 0.1f, Rgba8::RED);
		AddVertsForArrow2D(raycastVsLineSegmentVerts, closestRaycast.m_impactPosition, closestRaycast.m_impactPosition + closestRaycast.m_impactNormal * 5.f, 2.f, 0.1f, Rgba8::YELLOW);
		AddVertsForDisc2D(raycastVsLineSegmentVerts, closestRaycast.m_impactPosition, 0.3f, Rgba8::WHITE);
	}
	else
	{
		AddVertsForArrow2D(raycastVsLineSegmentVerts, m_raycastStartPosition, m_raycastEndPosition, 2.f, 0.1f, Rgba8::GREEN);
	}

	g_renderer->SetBlendMode(BlendMode::ALPHA);
	g_renderer->SetDepthMode(DepthMode::DISABLED);
	g_renderer->SetModelConstants();
	g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
	g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->BindShader(nullptr);
	g_renderer->BindTexture(nullptr);
	g_renderer->DrawVertexArray(raycastVsLineSegmentVerts);

	g_renderer->EndCamera(m_worldCamera);
}

void VisualTestRaycastVsLineSegments::Randomize()
{
	m_lineSegments.clear();

	for (int lineSegmentIndex = 0; lineSegmentIndex < NUM_LINE_SEGMENTS; lineSegmentIndex++)
	{
		float lineSegmentLength = g_RNG->RollRandomFloatInRange(LINE_SEGMENT_MIN_LENGTH, LINE_SEGMENT_MAX_LENGTH);
		float lineSegmentOrientationDegrees = g_RNG->RollRandomFloatInRange(0.f, 360.f);
		LineSegment2D lineSegment;
		lineSegment.m_start = g_RNG->RollRandomVec2InRange(0.f, WORLD_SIZE_X, 0.f, WORLD_SIZE_Y);
		lineSegment.m_end = lineSegment.m_start + Vec2::MakeFromPolarDegrees(lineSegmentOrientationDegrees, lineSegmentLength);

		m_lineSegments.push_back(lineSegment);
	}
}