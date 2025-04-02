#include "Game/VisualTestRaycastVsTiles.hpp"

#include "Engine/Core/HeatMaps/TileHeatMap.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"

constexpr float TILEWORLD_X = 40.f;
constexpr float TILEWORLD_Y = 20.f;
constexpr float SOLID_TILE_HEAT_VALUE = 999.f;

VisualTestRaycastVsTiles::VisualTestRaycastVsTiles()
{
	g_input->SetCursorMode(false, false);

	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	m_worldCamera.SetOrthoView(Vec2::ZERO, Vec2(TILEWORLD_X, TILEWORLD_Y));

	Randomize();
}

void VisualTestRaycastVsTiles::Update(float deltaSeconds)
{
	float pointsMovementSpeed = 4.f;
	
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
		AABB2 worldBox(m_worldCamera.GetOrthoBottomLeft(), m_worldCamera.GetOrthoTopRight());
		Vec2 cursorScreenPos = worldBox.GetPointAtUV(normalizedCursorPos);
		m_raycastStartPosition = cursorScreenPos;
	}
	if (g_input->IsKeyDown(KEYCODE_RMB))
	{
		Vec2 normalizedCursorPos = g_input->GetCursorNormalizedPosition();
		AABB2 worldBox(m_worldCamera.GetOrthoBottomLeft(), m_worldCamera.GetOrthoTopRight());
		Vec2 cursorScreenPos = worldBox.GetPointAtUV(normalizedCursorPos);
		m_raycastEndPosition = cursorScreenPos;
	}

	DebugAddMessage("F8 to randomize; LMB/RMB set ray start/end; WASD to move start, IJKL to move end, arrow keys to move ray", 0.f, Rgba8::CYAN, Rgba8::CYAN);
	DebugAddMessage("Mode [F6/F7 for Prev/Next]: Raycast vs 2D Tile Grid", 0.f, Rgba8::YELLOW, Rgba8::YELLOW);
}

void VisualTestRaycastVsTiles::Render() const
{
	g_renderer->BeginCamera(m_worldCamera);

	constexpr float LINE_THICKNESS = 0.0005f * TILEWORLD_X;

	int dimensionX = RoundDownToInt(TILEWORLD_X);
	int dimensionY = RoundDownToInt(TILEWORLD_Y);

	std::vector<Vertex_PCU> raycastVerts;
	int estimatedTileVertexes = 3 * 2 * dimensionX * dimensionY;
	raycastVerts.reserve(estimatedTileVertexes);

	m_tileHeatmap->AddVertsForDebugDraw(raycastVerts, AABB2(m_worldCamera.GetOrthoBottomLeft(), m_worldCamera.GetOrthoTopRight()), FloatRange::ZERO_TO_ONE, Rgba8::BLACK, Rgba8::BLACK, SOLID_TILE_HEAT_VALUE, Rgba8::ROYAL_BLUE);
	
	for (int tileY = 0; tileY < dimensionY; tileY++)
	{
		AddVertsForLineSegment2D(raycastVerts, Vec2(0.f, static_cast<float>(tileY)), Vec2(static_cast<float>(dimensionX), static_cast<float>(tileY)), LINE_THICKNESS, Rgba8::GRAY);
	}

	for (int tileX = 0; tileX < dimensionX; tileX++)
	{
		AddVertsForLineSegment2D(raycastVerts, Vec2(static_cast<float>(tileX), 0.f), Vec2(static_cast<float>(tileX), static_cast<float>(dimensionY)), LINE_THICKNESS, Rgba8::GRAY);
	}

	Vec2 raycastDirection = (m_raycastEndPosition - m_raycastStartPosition).GetNormalized();
	float raycastMaxLength = GetDistance2D(m_raycastStartPosition, m_raycastEndPosition);
	RaycastResult2D raycastResult = m_tileHeatmap->Raycast(m_raycastStartPosition, raycastDirection, raycastMaxLength);
	Vec2 rayEndpoint = m_raycastEndPosition;
	Rgba8 rayColor = Rgba8::GREEN;
	if (raycastResult.m_didImpact)
	{
		rayEndpoint = raycastResult.m_impactPosition;
		rayColor = Rgba8::RED;
	}

	AddVertsForArrow2D(raycastVerts, m_raycastStartPosition, m_raycastEndPosition, 0.1f, LINE_THICKNESS * 2.f, Rgba8::GRAY);
	AddVertsForArrow2D(raycastVerts, m_raycastStartPosition, rayEndpoint, 0.1f, LINE_THICKNESS * 2.f, rayColor);

	if (raycastResult.m_didImpact)
	{
		AddVertsForDisc2D(raycastVerts, raycastResult.m_impactPosition, 0.05f, Rgba8::WHITE);
		AddVertsForArrow2D(raycastVerts, raycastResult.m_impactPosition, raycastResult.m_impactPosition + raycastResult.m_impactNormal * 0.5f, 0.1f, LINE_THICKNESS * 2.f, Rgba8::YELLOW);
	}

	g_renderer->SetBlendMode(BlendMode::ALPHA);
	g_renderer->SetDepthMode(DepthMode::DISABLED);
	g_renderer->SetModelConstants();
	g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
	g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->BindShader(nullptr);
	g_renderer->BindTexture(nullptr);
	g_renderer->DrawVertexArray(raycastVerts);

	g_renderer->EndCamera(m_worldCamera);
}

void VisualTestRaycastVsTiles::Randomize()
{
	int dimensionX = RoundDownToInt(TILEWORLD_X);
	int dimensionY = RoundDownToInt(TILEWORLD_Y);
	m_tileHeatmap = new TileHeatMap(IntVec2(dimensionX, dimensionY));

	std::vector<float> heatValues;
	heatValues.resize(dimensionX * dimensionY);
	for (int tileY = 0; tileY < dimensionY; tileY++)
	{
		for (int tileX = 0; tileX < dimensionX; tileX++)
		{
			int tileIndex = tileX + tileY * dimensionX;
			heatValues[tileIndex] = g_RNG->RollRandomChance(0.1f) ? SOLID_TILE_HEAT_VALUE : 1.f;
		}
	}

	m_tileHeatmap->SetAllValues(heatValues);
}
