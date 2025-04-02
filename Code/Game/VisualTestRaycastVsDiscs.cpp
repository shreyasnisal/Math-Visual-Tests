#include "Game/VisualTestRaycastVsDiscs.hpp"

#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"

static constexpr int NUM_DISCS = 10;
constexpr float RAYCAST_DISC_MIN_RADIUS = 1.f;
constexpr float RAYCAST_DISC_MAX_RADIUS = 10.f;

VisualTestRaycastVsDiscs::VisualTestRaycastVsDiscs()
{
	g_input->SetCursorMode(false, false);

	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	m_worldCamera.SetOrthoView(Vec2::ZERO, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));

	Randomize();
}

void VisualTestRaycastVsDiscs::Update(float deltaSeconds)
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
	DebugAddMessage("Mode [F6/F7 for Prev/Next]: Raycast vs Discs", 0.f, Rgba8::YELLOW, Rgba8::YELLOW);
}

void VisualTestRaycastVsDiscs::Render() const
{
	g_renderer->BeginCamera(m_worldCamera);

	std::vector<Vertex_PCU> raycastVsDiscsVerts;
	raycastVsDiscsVerts.reserve(500);

	RaycastResult2D raycastResult;

	Vec2 raycastDirection = (m_raycastEndPosition - m_raycastStartPosition).GetNormalized();
	float raycastMaxLength = GetDistance2D(m_raycastStartPosition, m_raycastEndPosition);

	for (int discIndex = 0; discIndex < NUM_DISCS; discIndex++)
	{
		Disc2D const& disc = m_discs[discIndex];
		AddVertsForDisc2D(raycastVsDiscsVerts, disc.m_center, disc.m_radius, Rgba8::NAVY);
	}

	Disc2D closestImpactDisc;
	RaycastResult2D closestRaycast;
	closestRaycast.m_impactDistance = 99999.f;

	for (int discIndex = 0; discIndex < NUM_DISCS; discIndex++)
	{
		Disc2D const& disc = m_discs[discIndex];

		raycastResult = RaycastVsDisc2D(m_raycastStartPosition, raycastDirection, raycastMaxLength, disc.m_center, disc.m_radius);

		if (raycastResult.m_didImpact)
		{
			if (raycastResult.m_impactDistance < closestRaycast.m_impactDistance)
			{
				closestRaycast = raycastResult;
				closestImpactDisc = disc;
			}
		}
	}

	if (closestRaycast.m_didImpact)
	{
		AddVertsForDisc2D(raycastVsDiscsVerts, closestImpactDisc.m_center, closestImpactDisc.m_radius, Rgba8::DODGER_BLUE);
		AddVertsForArrow2D(raycastVsDiscsVerts, m_raycastStartPosition, m_raycastEndPosition, 2.f, 0.2f, Rgba8::GRAY);
		AddVertsForArrow2D(raycastVsDiscsVerts, m_raycastStartPosition, closestRaycast.m_impactPosition, 2.f, 0.2f, Rgba8::RED);
		AddVertsForArrow2D(raycastVsDiscsVerts, closestRaycast.m_impactPosition, closestRaycast.m_impactPosition + closestRaycast.m_impactNormal * 50.f, 2.f, 0.1f, Rgba8::YELLOW);
		AddVertsForDisc2D(raycastVsDiscsVerts, closestRaycast.m_impactPosition, 0.4f, Rgba8::WHITE);
	}
	else
	{
		AddVertsForArrow2D(raycastVsDiscsVerts, m_raycastStartPosition, m_raycastEndPosition, 2.f, 0.2f, Rgba8::GREEN);
	}

	g_renderer->SetBlendMode(BlendMode::ALPHA);
	g_renderer->SetDepthMode(DepthMode::DISABLED);
	g_renderer->SetModelConstants();
	g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
	g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->BindShader(nullptr);
	g_renderer->BindTexture(nullptr);
	g_renderer->DrawVertexArray(raycastVsDiscsVerts);

	g_renderer->EndCamera(m_worldCamera);
}

void VisualTestRaycastVsDiscs::Randomize()
{
	m_discs.clear();

	for (int discIndex = 0; discIndex < NUM_DISCS; discIndex++)
	{
		m_discs.push_back(Disc2D());
		Disc2D& disc = m_discs[discIndex];
		disc.m_radius = g_RNG->RollRandomFloatInRange(RAYCAST_DISC_MIN_RADIUS, RAYCAST_DISC_MAX_RADIUS);
		disc.m_center = g_RNG->RollRandomVec2InRange(disc.m_radius, m_worldCamera.GetOrthoTopRight().x - disc.m_radius, disc.m_radius, m_worldCamera.GetOrthoTopRight().y - disc.m_radius);
	}
}
