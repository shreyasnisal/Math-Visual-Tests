#include "Game/VisualTestRaycastVsAABB2.hpp"

#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"

static constexpr int NUM_AABBS = 10;
constexpr float BOX_MIN_DIMENSION = 5.f;
constexpr float BOX_MAX_DIMENSION = 10.f;

VisualTestRaycastVsAABB2::VisualTestRaycastVsAABB2()
{
	g_input->SetCursorMode(false, false);

	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	m_worldCamera.SetOrthoView(Vec2::ZERO, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));

	Randomize();
}

void VisualTestRaycastVsAABB2::Update(float deltaSeconds)
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
	DebugAddMessage("Mode [F6/F7 for Prev/Next]: Raycast vs Axis-Aligned Bounding Boxes 2D", 0.f, Rgba8::YELLOW, Rgba8::YELLOW);
}

void VisualTestRaycastVsAABB2::Render() const
{
	g_renderer->BeginCamera(m_worldCamera);

	std::vector<Vertex_PCU> raycastVsAABB2Verts;
	for (int aabbIndex = 0; aabbIndex < NUM_AABBS; aabbIndex++)
	{
		AddVertsForAABB2(raycastVsAABB2Verts, m_aabbs[aabbIndex], Rgba8::NAVY);
	}

	RaycastResult2D raycastResult;

	Vec2 raycastDirection = (m_raycastEndPosition - m_raycastStartPosition).GetNormalized();
	float raycastMaxLength = GetDistance2D(m_raycastStartPosition, m_raycastEndPosition);

	AABB2 closestImpactBox;
	RaycastResult2D closestRaycast;
	closestRaycast.m_impactDistance = FLT_MAX;

	for (int aabbIndex = 0; aabbIndex < NUM_AABBS; aabbIndex++)
	{
		AABB2 const& boundingBox = m_aabbs[aabbIndex];

		raycastResult = RaycastVsAABB2(m_raycastStartPosition, raycastDirection, raycastMaxLength, boundingBox);

		if (raycastResult.m_didImpact)
		{
			if (raycastResult.m_impactDistance < closestRaycast.m_impactDistance)
			{
				closestRaycast = raycastResult;
				closestImpactBox = boundingBox;
			}
		}
	}

	if (closestRaycast.m_didImpact)
	{
		AddVertsForAABB2(raycastVsAABB2Verts, closestImpactBox, Rgba8::DODGER_BLUE);
		AddVertsForArrow2D(raycastVsAABB2Verts, m_raycastStartPosition, m_raycastEndPosition, 2.f, 0.1f, Rgba8::GRAY);
		AddVertsForArrow2D(raycastVsAABB2Verts, m_raycastStartPosition, closestRaycast.m_impactPosition, 2.f, 0.1f, Rgba8::RED);
		AddVertsForArrow2D(raycastVsAABB2Verts, closestRaycast.m_impactPosition, closestRaycast.m_impactPosition + closestRaycast.m_impactNormal * 5.f, 2.f, 0.1f, Rgba8::YELLOW);
		AddVertsForDisc2D(raycastVsAABB2Verts, closestRaycast.m_impactPosition, 0.3f, Rgba8::WHITE);
	}
	else
	{
		AddVertsForArrow2D(raycastVsAABB2Verts, m_raycastStartPosition, m_raycastEndPosition, 2.f, 0.1f, Rgba8::GREEN);
	}

	g_renderer->SetBlendMode(BlendMode::ALPHA);
	g_renderer->SetDepthMode(DepthMode::DISABLED);
	g_renderer->SetModelConstants();
	g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
	g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->BindTexture(nullptr);
	g_renderer->DrawVertexArray(raycastVsAABB2Verts);

	g_renderer->EndCamera(m_worldCamera);
}

void VisualTestRaycastVsAABB2::Randomize()
{
	m_aabbs.clear();

	for (int aabbIndex = 0; aabbIndex < NUM_AABBS; aabbIndex++)
	{
		float axisAlignedBoxWidth = g_RNG->RollRandomFloatInRange(BOX_MIN_DIMENSION, BOX_MAX_DIMENSION);
		float axisAlignedBoxHeight = g_RNG->RollRandomFloatInRange(BOX_MIN_DIMENSION, BOX_MAX_DIMENSION);
		Vec2 axisAlignedBoxBL = g_RNG->RollRandomVec2InRange(axisAlignedBoxWidth, WORLD_SIZE_X - axisAlignedBoxWidth, axisAlignedBoxHeight, WORLD_SIZE_Y - axisAlignedBoxHeight);
		Vec2 axisAlignedBoxTR = axisAlignedBoxBL + Vec2(axisAlignedBoxWidth, axisAlignedBoxHeight);

		m_aabbs.push_back(AABB2(axisAlignedBoxBL, axisAlignedBoxTR));
	}
}