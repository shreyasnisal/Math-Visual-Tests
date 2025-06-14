#include "Game/VisualTestConvexScene.hpp"

#include "Game/App.hpp"

#include "Engine/Core/BufferWriter.hpp"
#include "Engine/Core/BufferParser.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"


VisualTestConvexScene::~VisualTestConvexScene()
{
	UnsubscribeEventCallbackFunction("SaveConvexScene", Command_SaveScene);
	UnsubscribeEventCallbackFunction("LoadConvexScene", Command_LoadScene);
}

VisualTestConvexScene::VisualTestConvexScene()
{
	m_worldCamera.SetOrthoView(m_worldBounds.m_mins, m_worldBounds.m_maxs);
	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	g_input->SetCursorMode(false, false);

	m_gameClock = new Clock(Clock::GetSystemClock());

	SubscribeEventCallbackFunction("SaveConvexScene", Command_SaveScene, "Save current scene to GHCS file (help for arguments)");
	SubscribeEventCallbackFunction("LoadConvexScene", Command_LoadScene, "Load scene from GHCS file (help for arguments)");

	Randomize();
}

void VisualTestConvexScene::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	HandleInput();

	if (m_raycastsPerformedInLastTest != 0)
	{
		DebugAddMessage(Stringf("Time taken for %d raycasts: %.2f ms, Average impact distance: %.2f units", m_raycastsPerformedInLastTest, m_totalRaycastTimeMs, m_averageRaycastImpactDistance), 0.f, Rgba8::WHITE, Rgba8::WHITE);
	}
	DebugAddMessage(Stringf("T = Fire raycasts"), 0.f, Rgba8::CYAN, Rgba8::CYAN);
	DebugAddMessage(Stringf("Num Polys [Q/E] = %d; Num Raycasts [Z/C] = %d; Optimization [F9] = %s;", m_currentNumPolys, m_currentNumRaycasts, GetOptimizationModeStr(m_currentOptimizationMode).c_str()), 0.f, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddMessage(Stringf("F1 = Toggle bounding disc debug draw (per polygon); F2 = Toggle shape translucency; F4 = Toggle bit buckets debug draw"), 0.f, Rgba8::CYAN, Rgba8::CYAN);
	DebugAddMessage(Stringf("F8 = Reset; LMB/RMB = Move raycst start/end; LMB = Drag poly; A/D = Rotate; W/S = Scale"), 0.f, Rgba8::CYAN, Rgba8::CYAN);
	DebugAddMessage("Mode [F6/F7 = Prev/Next]: Convex Scene (2D)", 0.f, Rgba8::YELLOW, Rgba8::YELLOW);

	if (m_boundingDiscs.empty() && m_currentOptimizationMode == OptimizationMode::NARROW_PHASE_BOUNDING_DISC_ONLY)
	{
		DebugAddMessage("Narrow phase optimization unavailable since no bounding discs were loaded. No optimization will be performed!", 0.f, Rgba8::RED, Rgba8::RED);
	}
	if (m_boundingDiscs.empty() && m_currentOptimizationMode == OptimizationMode::NARROW_AND_BROAD_PHASE)
	{
		DebugAddMessage("Narrow phase optimization unavailable since no bounding discs were loaded. Only broad phase optimization will be performed!", 0.f, Rgba8::RED, Rgba8::RED);
	}

	m_worldCamera.SetOrthoView(m_worldBounds.m_mins, m_worldBounds.m_maxs);
	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void VisualTestConvexScene::Render() const
{
	constexpr float POLY_OUTLINE_THICKNESS = 0.4f;
	constexpr float BOUNDING_DISC_THICKNESS = 0.2f;

	Rgba8 const& outlineColor = Rgba8::ROYAL_BLUE;
	Rgba8 const& fillColor = m_drawWithTranslucentFill ? Rgba8(Rgba8::DEEP_SKY_BLUE.r, Rgba8::DEEP_SKY_BLUE.g, Rgba8::DEEP_SKY_BLUE.b, 127) : Rgba8::DEEP_SKY_BLUE;

	std::vector<Vertex_PCU> vertexes;

	for (int polyIndex = 0; polyIndex < m_convexPolys.size(); polyIndex++)
	{
		AddOutlineVertsForConvexPoly2(vertexes, m_convexPolys[polyIndex], POLY_OUTLINE_THICKNESS  * m_sceneBounds.GetDimensions().y / WORLD_SIZE_Y, outlineColor);
	}

	for (int polyIndex = 0; polyIndex < m_convexPolys.size(); polyIndex++)
	{
		AddVertsForConvexPoly2(vertexes, m_convexPolys[polyIndex], fillColor);
	}

	for (int polyIndex = 0; polyIndex < m_convexPolys.size(); polyIndex++)
	{
		if (m_boundingDiscs.size() <= polyIndex)
		{
			break;
		}

		if (m_boundingDiscs[polyIndex].m_visible)
		{
			AddVertsForRing2D(vertexes, m_boundingDiscs[polyIndex].m_center, m_boundingDiscs[polyIndex].m_radius, BOUNDING_DISC_THICKNESS * m_sceneBounds.GetDimensions().y / WORLD_SIZE_Y, Rgba8::MAGENTA);
		}
	}

	if (m_currentNumPolys == 1)
	{
		ConvexHull2 const& convexHull = m_convexPolys[0];
		std::vector<Plane2> planes = convexHull.GetPlanes();
		for (int planeIndex = 0; planeIndex < (int)planes.size(); planeIndex++)
		{
			Vec2 planeCenter = planes[planeIndex].m_distanceFromOriginAlongNormal * planes[planeIndex].m_normal;
			Vec2 lineDirection = planes[planeIndex].m_normal.GetRotated90Degrees();
			AddVertsForLineSegment2D(vertexes, planeCenter - lineDirection * 1000.f  * m_sceneBounds.GetDimensions().y / WORLD_SIZE_Y, planeCenter + lineDirection * 1000.f * m_sceneBounds.GetDimensions().y / WORLD_SIZE_Y, 0.1f * m_sceneBounds.GetDimensions().y / WORLD_SIZE_Y, Rgba8::MAGENTA);
		}
	}

	if (m_drawBitBucketGrid)
	{
		for (int x = 0; x <= BIT_BUCKET_GRID_SIZE_X; x++)
		{
			AddVertsForLineSegment2D(vertexes, Vec2((float)x * WORLD_SIZE_X / BIT_BUCKET_GRID_SIZE_X, 0.f), Vec2((float)x * WORLD_SIZE_X / BIT_BUCKET_GRID_SIZE_X, WORLD_SIZE_Y), 0.1f, Rgba8::YELLOW);
		}
		for (int y = 0; y <= BIT_BUCKET_GRID_SIZE_Y; y++)
		{
			AddVertsForLineSegment2D(vertexes, Vec2(0.f, (float)y * WORLD_SIZE_Y / BIT_BUCKET_GRID_SIZE_Y), Vec2(WORLD_SIZE_X, (float)y * WORLD_SIZE_Y / BIT_BUCKET_GRID_SIZE_Y), 0.1f, Rgba8::YELLOW);
		}
	}

	if (m_hoveredConvexPolyIndex != -1)
	{
		AddOutlineVertsForConvexPoly2(vertexes, m_convexPolys[m_hoveredConvexPolyIndex], POLY_OUTLINE_THICKNESS, outlineColor);
		AddVertsForConvexPoly2(vertexes, m_convexPolys[m_hoveredConvexPolyIndex], Rgba8::DODGER_BLUE);
	}

	// Add visible raycast verts
	Vec2 rayFwd = (m_visibleRaycastEnd - m_visibleRaycastStart).GetNormalized();
	float rayMaxDistance = (m_visibleRaycastEnd - m_visibleRaycastStart).GetLength();
	RaycastResult2D closestRaycastResult;
	closestRaycastResult.m_rayStartPosition = m_visibleRaycastStart;
	closestRaycastResult.m_rayForwardNormal = rayFwd;
	closestRaycastResult.m_impactDistance = rayMaxDistance;
	for (int polyIndex = 0; polyIndex < (int)m_convexPolys.size(); polyIndex++)
	{
		RaycastResult2D raycastVsHullResult = RaycastVsConvexHull2_WithDebugDrawWhenForSimpleHull(m_visibleRaycastStart, rayFwd, rayMaxDistance, m_convexHulls[polyIndex], vertexes);
		if (raycastVsHullResult.m_didImpact && raycastVsHullResult.m_impactDistance < closestRaycastResult.m_impactDistance)
		{
			closestRaycastResult = raycastVsHullResult;
		}
	}

	if (closestRaycastResult.m_didImpact)
	{
		AddVertsForArrow2D(vertexes, m_visibleRaycastStart, m_visibleRaycastEnd, m_sceneBounds.GetDimensions().y * 0.01f, m_sceneBounds.GetDimensions().y * 0.002f, Rgba8::GRAY);
		AddVertsForArrow2D(vertexes, m_visibleRaycastStart, closestRaycastResult.m_impactPosition, m_sceneBounds.GetDimensions().y * 0.01f, m_sceneBounds.GetDimensions().y * 0.002f, Rgba8::GREEN);
		AddVertsForArrow2D(vertexes, closestRaycastResult.m_impactPosition, closestRaycastResult.m_impactPosition + closestRaycastResult.m_impactNormal * 10.f, m_sceneBounds.GetDimensions().y * 0.01f, m_sceneBounds.GetDimensions().y * 0.002f, Rgba8::YELLOW);
		AddVertsForDisc2D(vertexes, closestRaycastResult.m_impactPosition, 0.4f, Rgba8::WHITE);
	}
	else
	{
		AddVertsForArrow2D(vertexes, m_visibleRaycastStart, m_visibleRaycastEnd, m_sceneBounds.GetDimensions().y * 0.01f, m_sceneBounds.GetDimensions().y * 0.002f, Rgba8::RED);
	}

	// Render letterbox/pillarbox borders last
	AddVertsForAABB2(vertexes, AABB2(Vec2(m_worldCamera.GetOrthoBottomLeft().x, m_worldCamera.GetOrthoBottomLeft().y), Vec2(m_worldCamera.GetOrthoTopRight().x, 0.0)), Rgba8::GRAY);
	AddVertsForAABB2(vertexes, AABB2(Vec2(m_worldCamera.GetOrthoBottomLeft().x, m_worldCamera.GetOrthoBottomLeft().y), Vec2(0.0, m_worldCamera.GetOrthoTopRight().y)), Rgba8::GRAY);
	AddVertsForAABB2(vertexes, AABB2(Vec2(m_sceneBounds.m_maxs.x, 0.0), Vec2(m_worldCamera.GetOrthoTopRight().x, m_worldCamera.GetOrthoTopRight().y)), Rgba8::GRAY);
	AddVertsForAABB2(vertexes, AABB2(Vec2(0.0, m_sceneBounds.m_maxs.y), Vec2(m_worldCamera.GetOrthoTopRight().x, m_worldCamera.GetOrthoTopRight().y)), Rgba8::GRAY);

	g_renderer->BeginCamera(m_worldCamera);
	g_renderer->BeginRenderEvent("Convex Scene");
	g_renderer->BindShader(nullptr);
	g_renderer->BindTexture(nullptr);
	g_renderer->SetModelConstants();
	g_renderer->SetBlendMode(BlendMode::ALPHA);
	g_renderer->SetDepthMode(DepthMode::DISABLED);
	g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
	g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->DrawVertexArray(vertexes);
	g_renderer->EndRenderEvent("Convex Scene");
	g_renderer->EndCamera(m_worldCamera);
}

void VisualTestConvexScene::Randomize()
{
	m_selectedConvexPolyIndex = -1;
	m_hoveredConvexPolyIndex = -1;

	m_convexHulls.clear();
	m_convexPolys.clear();
	m_boundingDiscs.clear();

	for (int polyIndex = 0; polyIndex < m_currentNumPolys; polyIndex++)
	{
		// Create a bounding disc that is completely within the scene
		m_boundingDiscs.push_back(BoundingDisc());
		BoundingDisc& boundingDisc = m_boundingDiscs[polyIndex];
		boundingDisc.m_radius = g_RNG->RollRandomFloatInRange(BOUNDING_DISC_MIN_RADIUS * m_sceneBounds.GetDimensions().y / WORLD_SIZE_Y, BOUNDING_DISC_MAX_RADIUS * m_sceneBounds.GetDimensions().y / WORLD_SIZE_Y);
		boundingDisc.m_center = g_RNG->RollRandomVec2InRange(boundingDisc.m_radius, m_worldCamera.GetOrthoTopRight().x - boundingDisc.m_radius, boundingDisc.m_radius, m_worldCamera.GetOrthoTopRight().y - boundingDisc.m_radius);

		// Create convex poly vertexes on bounding disc
		float currentTheta = 0.f;
		std::vector<Vec2> polyVertexes;
		while (currentTheta < 360.f)
		{
			currentTheta += g_RNG->RollRandomFloatInRange(MIN_THETA_INCREMENT_FOR_POLY_VERTEX, MAX_THETA_INCREMENT_FOR_POLY_VERTEX);
			if (currentTheta > 360.f)
			{
				break;
			}

			Vec2 vertexPosition = boundingDisc.m_center + Vec2::MakeFromPolarDegrees(currentTheta, boundingDisc.m_radius);
			polyVertexes.push_back(vertexPosition);
		}
		m_convexPolys.push_back(ConvexPoly2(polyVertexes));
	}

	GenerateHullsForAllPolys();
}

void VisualTestConvexScene::HandleInput()
{
	float deltaSeconds = m_gameClock->GetDeltaSeconds();

	constexpr float ROTATION_PER_SECOND = 30.f;
	constexpr float SCALING_PER_SECOND = 0.1f;

	Vec2 const& cursorNormalizedPosition = g_input->GetCursorNormalizedPosition();
	Vec2 cursorWorldPosition = m_worldBounds.GetPointAtUV(cursorNormalizedPosition);
	//cursorWorldPosition.x = RangeMap(cursorNormalizedPosition.x, 0.f, 1.f, 0.f, WORLD_SIZE_X);
	//cursorWorldPosition.y = RangeMap(cursorNormalizedPosition.y, 0.f, 1.f, 0.f, WORLD_SIZE_Y);

	if (m_selectedConvexPolyIndex == -1 && !m_isMovingRaycast)
	{
		m_hoveredConvexPolyIndex = -1;
		for (int polyIndex = 0; polyIndex < m_convexPolys.size(); polyIndex++)
		{
			if (IsPointInsideConvexPoly2(cursorWorldPosition, m_convexPolys[polyIndex]))
			{
				m_hoveredConvexPolyIndex = polyIndex;
				break;
			}
		}
	}

	if (m_selectedConvexPolyIndex != -1)
	{
		if (m_boundingDiscs.size() >= m_hoveredConvexPolyIndex)
		{
			m_boundingDiscs[m_selectedConvexPolyIndex].m_center = cursorWorldPosition + m_selectedPolyOffsetFromCursorPosition;
		}
		for (int vertexIndex = 0; vertexIndex < m_convexPolys[m_selectedConvexPolyIndex].GetVertexes().size(); vertexIndex++)
		{
			m_convexPolys[m_selectedConvexPolyIndex].SetPositionForVertexAtIndex(cursorWorldPosition + m_vertexOffsetsFromCursorPosition[vertexIndex], vertexIndex);
		}
	}

	if (g_input->WasKeyJustPressed(KEYCODE_LMB))
	{
		if (m_hoveredConvexPolyIndex != -1)
		{
			m_selectedConvexPolyIndex = m_hoveredConvexPolyIndex;
			if (m_boundingDiscs.size() > m_hoveredConvexPolyIndex)
			{
				m_selectedPolyOffsetFromCursorPosition = m_boundingDiscs[m_selectedConvexPolyIndex].m_center - cursorWorldPosition;
			}

			std::vector<Vec2> selectedPolyVertexes = m_convexPolys[m_selectedConvexPolyIndex].GetVertexes();
			for (int vertexIndex = 0; vertexIndex < selectedPolyVertexes.size(); vertexIndex++)
			{
				m_vertexOffsetsFromCursorPosition.push_back(selectedPolyVertexes[vertexIndex] - cursorWorldPosition);
			}

			m_needToRegenerateBitMasks = true;
			m_unknownFileChunksLoaded.clear();
		}
		else
		{
			m_isMovingRaycast = true;
		}
	}
	if (g_input->WasKeyJustReleased(KEYCODE_LMB))
	{
		RegenerateHullForForPolyAtIndex(m_selectedConvexPolyIndex);
		m_isMovingRaycast = false;
		m_selectedConvexPolyIndex = -1;
		m_selectedPolyOffsetFromCursorPosition = Vec2::ZERO;
		m_vertexOffsetsFromCursorPosition.clear();
	}
	if (g_input->IsKeyDown(KEYCODE_LMB) && m_isMovingRaycast)
	{
		m_visibleRaycastStart = cursorWorldPosition;
	}
	if (g_input->WasKeyJustPressed(KEYCODE_RMB))
	{
		m_isMovingRaycast = true;
	}
	if (g_input->WasKeyJustReleased(KEYCODE_RMB))
	{
		m_isMovingRaycast = false;
	}
	if (g_input->IsKeyDown(KEYCODE_RMB) && m_isMovingRaycast)
	{
		m_visibleRaycastEnd = cursorWorldPosition;
	}

	// Rotation around cursor position
	if (g_input->IsKeyDown('A') && m_hoveredConvexPolyIndex != -1)
	{
		RotatePolyAtIndexAroundPointByDegrees(m_hoveredConvexPolyIndex, cursorWorldPosition, ROTATION_PER_SECOND * deltaSeconds);
	}
	if (g_input->IsKeyDown('D') && m_hoveredConvexPolyIndex != -1)
	{
		RotatePolyAtIndexAroundPointByDegrees(m_hoveredConvexPolyIndex, cursorWorldPosition, -ROTATION_PER_SECOND * deltaSeconds);
	}

	// Scaling around cursor position
	if (g_input->IsKeyDown('W') && m_hoveredConvexPolyIndex != -1)
	{
		ScalePolyAtIndexAroundPointByFactor(m_hoveredConvexPolyIndex, cursorWorldPosition, 1.f + SCALING_PER_SECOND * deltaSeconds);
	}
	if (g_input->IsKeyDown('S') && m_hoveredConvexPolyIndex != -1)
	{
		ScalePolyAtIndexAroundPointByFactor(m_hoveredConvexPolyIndex, cursorWorldPosition, 1.f - SCALING_PER_SECOND * deltaSeconds);
	}

	if (g_input->WasKeyJustPressed(KEYCODE_F1) && m_hoveredConvexPolyIndex != -1)
	{
		if (m_boundingDiscs.size() > m_hoveredConvexPolyIndex)
		{
			m_boundingDiscs[m_hoveredConvexPolyIndex].m_visible = !m_boundingDiscs[m_hoveredConvexPolyIndex].m_visible;
		}
	}
	if (g_input->WasKeyJustPressed(KEYCODE_F2))
	{
		m_drawWithTranslucentFill = !m_drawWithTranslucentFill;
	}
	if (g_input->WasKeyJustPressed(KEYCODE_F4))
	{
		m_drawBitBucketGrid = !m_drawBitBucketGrid;
	}
	if (g_input->WasKeyJustPressed(KEYCODE_F9))
	{
		m_currentOptimizationMode = OptimizationMode(((int)m_currentOptimizationMode + 1) % (int)OptimizationMode::NUM);
	}
	if (g_input->WasKeyJustPressed('E'))
	{
		if (m_currentNumPolys < NUM_MAX_POLYS)
		{
			m_needToRegenerateBitMasks = true;
			m_unknownFileChunksLoaded.clear();
			if (m_currentNumPolys == 0)
			{
				m_currentNumPolys = 1;
			}
			else
			{
				m_currentNumPolys *= 2;
			}
			Randomize();
		}
	}
	if (g_input->WasKeyJustPressed('Q'))
	{
		if (m_currentNumPolys > 0)
		{
			m_needToRegenerateBitMasks = true;
			m_unknownFileChunksLoaded.clear();
			m_currentNumPolys /= 2;
			Randomize();
		}
	}
	if (g_input->WasKeyJustPressed('Z'))
	{
		if (m_currentNumRaycasts > 1)
		{
			m_currentNumRaycasts /= 2;
		}
	}
	if (g_input->WasKeyJustPressed('C'))
	{
		if (m_currentNumRaycasts < NUM_MAX_RAYCASTS)
		{
			m_currentNumRaycasts *= 2;
		}
	}

	if (g_input->WasKeyJustPressed('T'))
	{
		if ((m_bitBucketMasks.empty() || m_needToRegenerateBitMasks) && (m_currentOptimizationMode == OptimizationMode::BROAD_PHASE_BIT_BUCKET_ONLY || m_currentOptimizationMode == OptimizationMode::NARROW_AND_BROAD_PHASE))
		{
			GenerateBitMasksForAllPolys();
		}
		GenerateRandomRaycasts();
		PerformAllTestRaycasts();
	}
}

void VisualTestConvexScene::RotatePolyAtIndexAroundPointByDegrees(int polyIndex, Vec2 const& point, float degrees)
{
	std::vector<Vec2> vertexes = m_convexPolys[polyIndex].GetVertexes();

	for (int vertexIndex = 0; vertexIndex < vertexes.size(); vertexIndex++)
	{
		Vec2 displacementPointToVertex = vertexes[vertexIndex] - point;
		Vec2 vertexRotatedPosition = point + displacementPointToVertex.GetRotatedDegrees(degrees);
		m_convexPolys[polyIndex].SetPositionForVertexAtIndex(vertexRotatedPosition, vertexIndex);
	}

	if ((int)m_boundingDiscs.size() > polyIndex)
	{
		Vec2 displacementPointToBoundingDiscCenter = m_boundingDiscs[polyIndex].m_center - point;
		m_boundingDiscs[polyIndex].m_center = point + displacementPointToBoundingDiscCenter.GetRotatedDegrees(degrees);
	}

	m_needToRegenerateBitMasks = true;
	m_unknownFileChunksLoaded.clear();
}

void VisualTestConvexScene::ScalePolyAtIndexAroundPointByFactor(int polyIndex, Vec2 const& point, float scalingFactor)
{
	std::vector<Vec2> vertexes = m_convexPolys[polyIndex].GetVertexes();

	for (int vertexIndex = 0; vertexIndex < vertexes.size(); vertexIndex++)
	{
		Vec2 displacementPointToVertex = vertexes[vertexIndex] - point;
		Vec2 vertexScaledPosition = point + displacementPointToVertex * scalingFactor;
		m_convexPolys[polyIndex].SetPositionForVertexAtIndex(vertexScaledPosition, vertexIndex);
	}

	if ((int)m_boundingDiscs.size() > polyIndex)
	{
		Vec2 displacementPointToBoundingDiscCenter = m_boundingDiscs[polyIndex].m_center - point;
		m_boundingDiscs[polyIndex].m_center = point + displacementPointToBoundingDiscCenter * scalingFactor;
		m_boundingDiscs[polyIndex].m_radius *= scalingFactor;
	}

	m_needToRegenerateBitMasks = true;
	m_unknownFileChunksLoaded.clear();
}

void VisualTestConvexScene::GenerateHullsForAllPolys()
{
	for (int polyIndex = 0; polyIndex < (int)m_convexPolys.size(); polyIndex++)
	{
		m_convexHulls.push_back(ConvexHull2(m_convexPolys[polyIndex]));
	}
}

void VisualTestConvexScene::RegenerateHullForForPolyAtIndex(int polyIndex)
{
	if (polyIndex == -1)
	{
		return;
	}

	m_convexHulls[polyIndex] = ConvexHull2(m_convexPolys[polyIndex]);
}

void VisualTestConvexScene::GenerateBitMasksForAllPolys()
{
	m_bitBucketMasks.clear();

	for (int polyIndex = 0; polyIndex < (int)m_convexPolys.size(); polyIndex++)
	{
		m_bitBucketMasks.push_back(0ull);
		ConvexPoly2 const& convexPoly = m_convexPolys[polyIndex];
		std::vector<Vec2> convexPolyVerts = convexPoly.GetVertexes();

		for (int vertexIndex = 0; vertexIndex < (int)convexPolyVerts.size(); vertexIndex++)
		{
			Vec2 const& vertexPosition = convexPolyVerts[vertexIndex];
			int tileIndexForVertexPosition = GetTileIndexForWorldPosition(vertexPosition);
			m_bitBucketMasks[polyIndex] |= 1ull << tileIndexForVertexPosition;

			Vec2 nextVertexPosition = convexPolyVerts[0];
			if (vertexIndex < (int)convexPolyVerts.size() - 1)
			{
				nextVertexPosition = convexPolyVerts[vertexIndex + 1];
			}

			std::vector<unsigned int> edgeTiles;
			GetAllTileIndexesForRaycastVsGrid(vertexPosition, (nextVertexPosition - vertexPosition).GetNormalized(), (nextVertexPosition - vertexPosition).GetLength(), edgeTiles);
			for (int edgeTileIndex = 0; edgeTileIndex < (int)edgeTiles.size(); edgeTileIndex++)
			{
				m_bitBucketMasks[polyIndex] |= 1ull << edgeTiles[edgeTileIndex];
			}
		}
	}
}

void VisualTestConvexScene::GenerateBoundingDiscsForAllPolys()
{
	m_boundingDiscs.clear();

	for (int polyIndex = 0; polyIndex < (int)m_convexPolys.size(); polyIndex++)
	{
		BoundingDisc disc;
		std::vector<Vec2> vertexes = m_convexPolys[polyIndex].GetVertexes();
		int numVertexes = m_convexPolys[polyIndex].GetVertexCount();
		for (int vertexIndex = 0; vertexIndex < numVertexes; vertexIndex++)
		{
			disc.m_center += vertexes[vertexIndex];
		}
		disc.m_center /= (float)numVertexes;

		float largestVertexDistanceFromCenter = 0.f;;
		for (int vertexIndex = 0; vertexIndex < numVertexes; vertexIndex++)
		{
			float vertexDistance = GetDistance2D(disc.m_center, vertexes[vertexIndex]);
			if (vertexDistance > largestVertexDistanceFromCenter)
			{
				largestVertexDistanceFromCenter = vertexDistance;
			}
		}

		disc.m_radius = largestVertexDistanceFromCenter;

		m_boundingDiscs.push_back(disc);
	}
}

RaycastResult2D VisualTestConvexScene::RaycastVsConvexHull2_WithDebugDrawWhenForSimpleHull(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDistance, ConvexHull2 const& convexHull, std::vector<Vertex_PCU>& verts) const
{
	bool drawColorCodedEntryExitPoints = false;
	if (m_currentNumPolys == 1)
	{
		drawColorCodedEntryExitPoints = true;
	}
	constexpr float COLOR_CODED_POINT_THICKNESS = 0.75f;

	RaycastResult2D result;
	result.m_rayStartPosition = startPos;
	result.m_rayForwardNormal = fwdNormal;
	result.m_rayMaxLength = FLT_MAX;

	// IsPointInside check
	if (IsPointInsideConvexHull2(startPos, convexHull))
	{
		result.m_didImpact = true;
		result.m_impactDistance = 0.f;
		result.m_impactPosition = startPos;
		result.m_impactNormal = -fwdNormal;
	}

	// Initialize lastEntry and firstExit variables
	float lastEntryDistance = -1.f;
	float firstExitDistance = maxDistance;
	RaycastResult2D lastEntryPlaneRaycastResult;

	// Loop over all planes and do raycast vs that plane
	// Update lastEntry or firstExit if applicable
	std::vector<Plane2> planes = convexHull.GetPlanes();
	for (int planeIndex = 0; planeIndex < (int)planes.size(); planeIndex++)
	{
		RaycastResult2D raycastVsPlaneResult = RaycastVsPlane2(startPos, fwdNormal, maxDistance, planes[planeIndex]);


		if (raycastVsPlaneResult.m_didImpact)
		{
			// Check if this is an entry or exit
			if (planes[planeIndex].IsPointBehind(startPos))
			{
				// Exit
				if (raycastVsPlaneResult.m_impactDistance < firstExitDistance)
				{
					firstExitDistance = raycastVsPlaneResult.m_impactDistance;
				}
				if (drawColorCodedEntryExitPoints)
				{
					AddVertsForDisc2D(verts, startPos + raycastVsPlaneResult.m_impactDistance * fwdNormal, COLOR_CODED_POINT_THICKNESS, Rgba8::MAROON);
				}
			}
			else if (planes[planeIndex].IsPointInFront(startPos))
			{
				// Entry
				if (raycastVsPlaneResult.m_impactDistance > lastEntryDistance)
				{
					lastEntryDistance = raycastVsPlaneResult.m_impactDistance;
					lastEntryPlaneRaycastResult = raycastVsPlaneResult;
				}
				if (drawColorCodedEntryExitPoints)
				{
					AddVertsForDisc2D(verts, startPos + raycastVsPlaneResult.m_impactDistance * fwdNormal, COLOR_CODED_POINT_THICKNESS, Rgba8::GREEN);
				}
			}
			else
			{
				if (DotProduct2D(fwdNormal, planes[planeIndex].m_normal) > 0.f)
				{
					// Exit
					if (raycastVsPlaneResult.m_impactDistance < firstExitDistance)
					{
						firstExitDistance = raycastVsPlaneResult.m_impactDistance;
					}
					if (drawColorCodedEntryExitPoints)
					{
						AddVertsForDisc2D(verts, startPos + raycastVsPlaneResult.m_impactDistance * fwdNormal, COLOR_CODED_POINT_THICKNESS, Rgba8::MAROON);
					}
				}
				else if (DotProduct2D(fwdNormal, planes[planeIndex].m_normal) < 0.f)
				{
					// Entry
					if (raycastVsPlaneResult.m_impactDistance > lastEntryDistance)
					{
						lastEntryDistance = raycastVsPlaneResult.m_impactDistance;
						lastEntryPlaneRaycastResult = raycastVsPlaneResult;
						lastEntryPlaneRaycastResult.m_impactNormal = planes[planeIndex].m_normal;
					}
					if (drawColorCodedEntryExitPoints)
					{
						AddVertsForDisc2D(verts, startPos + raycastVsPlaneResult.m_impactDistance * fwdNormal, COLOR_CODED_POINT_THICKNESS, Rgba8::GREEN);
					}
				}
			}
		}
	}

	if (drawColorCodedEntryExitPoints)
	{
		AddVertsForDisc2D(verts, startPos + firstExitDistance * fwdNormal, COLOR_CODED_POINT_THICKNESS, Rgba8::RED);
		AddVertsForDisc2D(verts, startPos + lastEntryDistance * fwdNormal, COLOR_CODED_POINT_THICKNESS, Rgba8::LIME);
	}

	// If there is no entry, miss!
	if (lastEntryDistance < 0.f)
	{
		return result;
	}

	// If there is no exit, check if end point is inside the hull
	if (firstExitDistance == FLT_MAX)
	{
		if (IsPointInsideConvexHull2(startPos + fwdNormal * maxDistance, convexHull))
		{
			result.m_didImpact = true;
			result.m_impactDistance = lastEntryDistance;
			result.m_impactPosition = startPos + lastEntryDistance * fwdNormal;
			result.m_impactNormal = lastEntryPlaneRaycastResult.m_impactNormal;
		}
		else
		{
			return result;
		}
	}

	// If lastEntry is before firstExit, could be a hit
	// Check that any point between the lastEntry and firstExit is inside the hull
	// Hit position is last entry
	if (lastEntryDistance < firstExitDistance)
	{
		Vec2 midPointBetweenEntryAndExit = ((startPos + lastEntryDistance * fwdNormal) + (startPos + firstExitDistance * fwdNormal)) * 0.5f;
		if (IsPointInsideConvexHull2(midPointBetweenEntryAndExit, convexHull))
		{
			result.m_didImpact = true;
			result.m_impactDistance = lastEntryDistance;
			result.m_impactPosition = startPos + lastEntryDistance * fwdNormal;
			result.m_impactNormal = lastEntryPlaneRaycastResult.m_impactNormal;
		}
	}

	return result;
}

void VisualTestConvexScene::GetAllTileIndexesForRaycastVsGrid(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDistance, std::vector<unsigned int>& out_tileIndexes) const
{
	IntVec2 currentTile = GetTileCoordsForWorldPosition(startPos);
	Vec2 rayStepSize = Vec2(fwdNormal.x != 0 ? (WORLD_SIZE_X / BIT_BUCKET_GRID_SIZE_X) / fabsf(fwdNormal.x) : 99999.f, fwdNormal.y != 0 ? (WORLD_SIZE_Y / BIT_BUCKET_GRID_SIZE_Y) / fabsf(fwdNormal.y) : 99999.f);
	Vec2 cumulativeRayLengthIn1D;
	IntVec2 directionXY;
	float totalRayLength = 0.f;

	if (fwdNormal.x < 0.f)
	{
		directionXY.x = -1;
		cumulativeRayLengthIn1D.x = (startPos.x * (BIT_BUCKET_GRID_SIZE_X / WORLD_SIZE_X) - static_cast<float>(currentTile.x)) * rayStepSize.x;
	}
	else
	{
		directionXY.x = 1;
		cumulativeRayLengthIn1D.x = (static_cast<float>(currentTile.x) + 1.f - startPos.x * (BIT_BUCKET_GRID_SIZE_X / WORLD_SIZE_X)) * rayStepSize.x;
	}

	if (fwdNormal.y < 0)
	{
		directionXY.y = -1;
		cumulativeRayLengthIn1D.y = (startPos.y * (BIT_BUCKET_GRID_SIZE_Y / WORLD_SIZE_Y) - static_cast<float>(currentTile.y)) * rayStepSize.y;
	}
	else
	{
		directionXY.y = 1;
		cumulativeRayLengthIn1D.y = (static_cast<float>(currentTile.y) + 1.f - startPos.y * (BIT_BUCKET_GRID_SIZE_Y / WORLD_SIZE_Y)) * rayStepSize.y;
	}

	while (totalRayLength < maxDistance)
	{
		if (currentTile.x < 0 || currentTile.y < 0 || currentTile.x > BIT_BUCKET_GRID_SIZE_X - 1 || currentTile.y > BIT_BUCKET_GRID_SIZE_Y)
		{
			return;
		}

		out_tileIndexes.push_back(GetTileIndexForTileCoords(currentTile));

		if (cumulativeRayLengthIn1D.x < cumulativeRayLengthIn1D.y)
		{
			currentTile.x += directionXY.x;
			totalRayLength = cumulativeRayLengthIn1D.x;
			cumulativeRayLengthIn1D.x += rayStepSize.x;
		}
		else
		{
			currentTile.y += directionXY.y;
			totalRayLength = cumulativeRayLengthIn1D.y;
			cumulativeRayLengthIn1D.y += rayStepSize.y;
		}
	}
}

void VisualTestConvexScene::GenerateRandomRaycasts()
{
	m_rayStartPositions.clear();
	m_rayFwdNormals.clear();
	m_rayMaxDistances.clear();

	for (int rayIndex = 0; rayIndex < m_currentNumRaycasts; rayIndex++)
	{
		m_rayStartPositions.push_back(g_RNG->RollRandomVec2InBox(AABB2(Vec2::ZERO, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y))));
		m_rayFwdNormals.push_back(Vec2::MakeFromPolarDegrees(g_RNG->RollRandomFloatInRange(0.f, 360.f)));
		m_rayMaxDistances.push_back(g_RNG->RollRandomFloatInRange(RAY_MIN_LENGTH, RAY_MAX_LENGTH));
	}
}

void VisualTestConvexScene::PerformAllTestRaycasts()
{
	m_raycastsPerformedInLastTest = m_currentNumRaycasts;
	int numHitRays = 0;
	float totalImpactDistance = 0.f;
	double raycastStartTimeSeconds = GetCurrentTimeSeconds();
	for (int rayIndex = 0; rayIndex < m_currentNumRaycasts; rayIndex++)
	{
		unsigned long long rayBitField = 0ull;
		float closestImpactDistance = FLT_MAX;

		if (m_currentOptimizationMode == OptimizationMode::BROAD_PHASE_BIT_BUCKET_ONLY || m_currentOptimizationMode == OptimizationMode::NARROW_AND_BROAD_PHASE)
		{
			std::vector<unsigned int> rayTileIndexes;
			GetAllTileIndexesForRaycastVsGrid(m_rayStartPositions[rayIndex], m_rayFwdNormals[rayIndex], m_rayMaxDistances[rayIndex], rayTileIndexes);
			for (int tileIndexIdx = 0; tileIndexIdx < (int)rayTileIndexes.size(); tileIndexIdx++)
			{
				rayBitField |= 1ull << rayTileIndexes[tileIndexIdx];
			}
		}

		for (int polyIndex = 0; polyIndex < m_currentNumPolys; polyIndex++)
		{
			if (m_currentOptimizationMode == OptimizationMode::BROAD_PHASE_BIT_BUCKET_ONLY || m_currentOptimizationMode == OptimizationMode::NARROW_AND_BROAD_PHASE)
			{
				if ((m_bitBucketMasks[polyIndex] & rayBitField) == 0)
				{
					continue;
				}
			}

			if (m_boundingDiscs.size() > polyIndex && (m_currentOptimizationMode == OptimizationMode::NARROW_PHASE_BOUNDING_DISC_ONLY || m_currentOptimizationMode == OptimizationMode::NARROW_AND_BROAD_PHASE))
			{
				RaycastResult2D raycastVsDiscResult = RaycastVsDisc2D(m_rayStartPositions[rayIndex], m_rayFwdNormals[rayIndex], m_rayMaxDistances[rayIndex], m_boundingDiscs[polyIndex].m_center, m_boundingDiscs[polyIndex].m_radius);
				if (!raycastVsDiscResult.m_didImpact)
				{
					continue;
				}
			}

			RaycastResult2D raycastVsConvexHullResult = RaycastVsConvexHull2(m_rayStartPositions[rayIndex], m_rayFwdNormals[rayIndex], m_rayMaxDistances[rayIndex], m_convexHulls[polyIndex]);
			if (raycastVsConvexHullResult.m_didImpact && raycastVsConvexHullResult.m_impactDistance < closestImpactDistance)
			{
				closestImpactDistance = raycastVsConvexHullResult.m_impactDistance;
			}
		}

		if (closestImpactDistance != FLT_MAX)
		{
			totalImpactDistance += closestImpactDistance;
			numHitRays++;
		}
	}
	double raycastEndTimeSeconds = GetCurrentTimeSeconds();
	m_totalRaycastTimeMs = (raycastEndTimeSeconds - raycastStartTimeSeconds) * 1000.f;
	m_averageRaycastImpactDistance = totalImpactDistance / (float)numHitRays;
}

int VisualTestConvexScene::GetTileIndexForWorldPosition(Vec2 const& worldPosition) const
{
	IntVec2 tileCoords(int(worldPosition.x * (float)BIT_BUCKET_GRID_SIZE_X / WORLD_SIZE_X), int(worldPosition.y * (float)BIT_BUCKET_GRID_SIZE_Y / WORLD_SIZE_Y));
	int tileIndex = tileCoords.x + tileCoords.y * BIT_BUCKET_GRID_SIZE_X;
	return tileIndex;
}

IntVec2 const VisualTestConvexScene::GetTileCoordsForWorldPosition(Vec2 const& worldPosition) const
{
	return IntVec2(int(worldPosition.x * (float)BIT_BUCKET_GRID_SIZE_X / WORLD_SIZE_X), int(worldPosition.y * (float)BIT_BUCKET_GRID_SIZE_Y / WORLD_SIZE_Y));

}

Vec2 const VisualTestConvexScene::GetWorldPositionForTileIndex(int tileIndex) const
{
	IntVec2 tileCoords = GetTileCoordsFromIndex(tileIndex);
	return Vec2((float)tileCoords.x * WORLD_SIZE_X / (float)BIT_BUCKET_GRID_SIZE_X, (float)tileCoords.y * WORLD_SIZE_Y / (float)BIT_BUCKET_GRID_SIZE_Y);
}

int VisualTestConvexScene::GetTileIndexForTileCoords(IntVec2 const& tileCoords) const
{
	return tileCoords.x + tileCoords.y * BIT_BUCKET_GRID_SIZE_X;
}

IntVec2 const VisualTestConvexScene::GetTileCoordsFromIndex(int tileIndex) const
{
	return IntVec2(tileIndex % BIT_BUCKET_GRID_SIZE_X, tileIndex / BIT_BUCKET_GRID_SIZE_X);
}

std::string GetOptimizationModeStr(OptimizationMode optimizationMode)
{
	switch (optimizationMode)
	{
		case OptimizationMode::NONE:								return "None";									break;
		case OptimizationMode::NARROW_PHASE_BOUNDING_DISC_ONLY:		return "Narrow Phase Only (Bounding Disc)";		break;
		case OptimizationMode::BROAD_PHASE_BIT_BUCKET_ONLY:			return "Broad Phase Only (Bit Buckets)";		break;
		case OptimizationMode::NARROW_AND_BROAD_PHASE:				return "Narrow and Broad Phase";				break;
	}

	return "";
}

void Append4ccCodeToWriter(char const* code, BufferWriter& writer)
{
	writer.AppendChar(code[0]);
	writer.AppendChar(code[1]);
	writer.AppendChar(code[2]);
	writer.AppendChar(code[3]);
}

char const* Parse4ccCodeFromParser(BufferParser& parser)
{
	char* code = new char[5];
	code[0] = parser.ParseChar();
	code[1] = parser.ParseChar();
	code[2] = parser.ParseChar();
	code[3] = parser.ParseChar();
	code[4] = 0;

	return code;
}

bool Command_SaveScene(EventArgs& args)
{
	bool help = args.GetValue("help", false);
	if (help)
	{
		g_console->AddLine("Command to save current scene to GHCS file.");
		g_console->AddLine("Arguments:");
		g_console->AddLine("\tname (string): The name of the file to save the scene to (without extension)");
		g_console->AddLine("\tsaveBoundingDiscs: Whether to save the optional bounding discs chunk");
		g_console->AddLine("\tsaveConvexHulls: Whether to save the optional convex hulls chunk");
		g_console->AddLine("\tsaveBitBuckets: Whether to save the optional bit buckets chunk");
		g_console->AddLine("\tendianMode: The endian mode to save the file in, must be either LITTLE or BIG");

		return false;
	}

	std::string sceneName = args.GetValue("name", "");
	if (sceneName.empty())
	{
		g_console->AddLine(DevConsole::ERROR, "No scene name provided for save- skipping save!");
		return false;
	}

	bool saveBoundingDiscs = args.GetValue("saveBoundingDiscs", false);
	bool saveConvexHulls = args.GetValue("saveConvexHulls", false);
	bool saveBitBuckets = args.GetValue("saveBitBuckets", false);

	std::vector<unsigned char> fileBuffer;
	BufferWriter writer(fileBuffer);

	std::string endianModeStr = args.GetValue("endianMode", "LITTLE");
	uint8_t endianModeCode = 0;
	if (!_stricmp(endianModeStr.c_str(), "LITTLE"))
	{
		endianModeCode = 1;
		writer.SetEndianMode(BufferEndian::LITTLE);
	}
	else if (!_stricmp(endianModeStr.c_str(), "BIG"))
	{
		endianModeCode = 2;
		writer.SetEndianMode(BufferEndian::BIG);
	}
	else
	{
		g_console->AddLine(DevConsole::ERROR, "Unknown endian mode specified. Skipping save!");
		return false;
	}

	std::string filePath = Stringf("Data/Scenes/%s.ghcs", sceneName.c_str());
	g_console->AddLine(DevConsole::INFO_MINOR, Stringf("Saving convex scene to %s", filePath.c_str()));

	Game* game = g_app->m_game;
	VisualTestConvexScene* convexScene = dynamic_cast<VisualTestConvexScene*>(game);

	// Header
	Append4ccCodeToWriter(CONVEX_SCENE_4CC_CODE, writer);
	writer.AppendByte(COHORT_ID);
	writer.AppendByte(MAJOR_VERSION);
	writer.AppendByte(MINOR_VERSION);
	writer.AppendByte(endianModeCode);
	int tocLocationInHeader = writer.GetAppendedSize();
	writer.AppendUint32(0);
	Append4ccCodeToWriter(CONVEX_HEADER_END_4CC_CODE, writer);

	uint8_t numChunksSaved = 0;
	uint32_t sceneInfoChunkStartLocation = 0;
	uint32_t sceneInfoChunkDataSize = 0;
	uint32_t convexPolysChunkStartLocation = 0;
	uint32_t convexPolysChunkDataSize = 0;
	uint32_t convexHullsChunkStartLocation = 0;
	uint32_t convexHullsChunkDataSize = 0;
	uint32_t boundingDiscsChunkStartLocation = 0;
	uint32_t boundingDiscsChunkDataSize = 0;
	uint32_t tiledBitRegionsChunkStartLocation = 0;
	uint32_t tiledBitRegionsChunkDataSize = 0;

	// Scene Info Chunk
	constexpr int SCENE_INFO_CHUNK_PAYLOAD_SIZE = 18;
	sceneInfoChunkStartLocation = writer.GetAppendedSize();
	Append4ccCodeToWriter(CONVEX_CHUNK_4CC_CODE, writer);
	writer.AppendByte((uint8_t)ChunkType::SCENE_INFO);
	writer.AppendByte(endianModeCode);
	writer.AppendUint32(SCENE_INFO_CHUNK_PAYLOAD_SIZE);
	writer.AppendFloat(convexScene->m_sceneBounds.m_mins.x);
	writer.AppendFloat(convexScene->m_sceneBounds.m_mins.y);
	writer.AppendFloat(convexScene->m_sceneBounds.m_maxs.x);
	writer.AppendFloat(convexScene->m_sceneBounds.m_maxs.y);
	writer.AppendUShort((uint16_t)convexScene->m_currentNumPolys);
	Append4ccCodeToWriter(CONVEX_CHUNK_END_4CC_CODE, writer);
	sceneInfoChunkDataSize = writer.GetAppendedSize() - sceneInfoChunkStartLocation;
	numChunksSaved++;

	// Convex Polys Chunk
	{
		convexPolysChunkStartLocation = writer.GetAppendedSize();
		Append4ccCodeToWriter(CONVEX_CHUNK_4CC_CODE, writer);
		writer.AppendByte((uint8_t)ChunkType::CONVEX_POLYS);
		writer.AppendByte(endianModeCode);
		int payloadLocation = writer.GetAppendedSize();
		writer.AppendUint32(0x00); // payload size will go here
		uint32_t payloadSize = 0;
		writer.AppendUShort((uint16_t)convexScene->m_currentNumPolys);
		payloadSize += sizeof(unsigned short);
		for (int polyIndex = 0; polyIndex < (int)convexScene->m_currentNumPolys; polyIndex++)
		{
			std::vector<Vec2> const vertexes = convexScene->m_convexPolys[polyIndex].GetVertexes();
			int numVertexes = convexScene->m_convexPolys[polyIndex].GetVertexCount();
			writer.AppendByte((uint8_t)numVertexes);
			payloadSize += sizeof(uint8_t);
			for (int vertexIndex = 0; vertexIndex < numVertexes; vertexIndex++)
			{
				writer.AppendVec2(vertexes[vertexIndex]);
				payloadSize += sizeof(Vec2);
			}
		}
		writer.OverwriteUint32AtPosition(payloadSize, payloadLocation);
		Append4ccCodeToWriter(CONVEX_CHUNK_END_4CC_CODE, writer);
		convexPolysChunkDataSize = writer.GetAppendedSize() - convexPolysChunkStartLocation;
		numChunksSaved++;
	}

	// Convex Hulls Chunk
	if (saveConvexHulls)
	{
		convexHullsChunkStartLocation = writer.GetAppendedSize();
		Append4ccCodeToWriter(CONVEX_CHUNK_4CC_CODE, writer);
		writer.AppendByte((uint8_t)ChunkType::CONVEX_HULLS);
		writer.AppendByte(endianModeCode);
		int payloadLocation = writer.GetAppendedSize();
		writer.AppendUint32(0x00); // payload size will go here
		uint32_t payloadSize = 0;
		writer.AppendUShort((uint16_t)convexScene->m_currentNumPolys);
		payloadSize += sizeof(unsigned short);
		for (int hullIndex = 0; hullIndex < (int)convexScene->m_currentNumPolys; hullIndex++)
		{
			std::vector<Plane2> const planes = convexScene->m_convexHulls[hullIndex].GetPlanes();
			int numPlanes = convexScene->m_convexHulls[hullIndex].GetPlanesCount();
			writer.AppendByte((uint8_t)numPlanes);
			payloadSize += sizeof(uint8_t);
			for (int planeIndex = 0; planeIndex < numPlanes; planeIndex++)
			{
				writer.AppendVec2(planes[planeIndex].m_normal);
				payloadSize += sizeof(Vec2);
				writer.AppendFloat(planes[planeIndex].m_distanceFromOriginAlongNormal);
				payloadSize += sizeof(float);
			}
		}
		writer.OverwriteUint32AtPosition(payloadSize, payloadLocation);
		Append4ccCodeToWriter(CONVEX_CHUNK_END_4CC_CODE, writer);
		convexHullsChunkDataSize = writer.GetAppendedSize() - convexHullsChunkStartLocation;
		numChunksSaved++;
	}

	// Bounding Discs Chunk
	if (saveBoundingDiscs)
	{
		if (convexScene->m_boundingDiscs.empty())
		{
			g_console->AddLine(DevConsole::WARNING, "Bounding discs not available since they were not loaded. Bounding disc chunk will be skipped in saving!");
			saveBoundingDiscs = false;
		}
		else
		{
			boundingDiscsChunkStartLocation = writer.GetAppendedSize();
			Append4ccCodeToWriter(CONVEX_CHUNK_4CC_CODE, writer);
			writer.AppendByte((uint8_t)ChunkType::BOUNDING_DISCS);
			writer.AppendByte(endianModeCode);
			int payloadLocation = writer.GetAppendedSize();
			writer.AppendUint32(0x00); // payload size will go here
			uint32_t payloadSize = 0;
			writer.AppendUShort((uint16_t)convexScene->m_currentNumPolys);
			payloadSize += sizeof(unsigned short);
			for (int discIndex = 0; discIndex < (int)convexScene->m_currentNumPolys; discIndex++)
			{
				writer.AppendVec2(convexScene->m_boundingDiscs[discIndex].m_center);
				payloadSize += sizeof(Vec2);
				writer.AppendFloat(convexScene->m_boundingDiscs[discIndex].m_radius);
				payloadSize += sizeof(float);
			}
			writer.OverwriteUint32AtPosition(payloadSize, payloadLocation);
			Append4ccCodeToWriter(CONVEX_CHUNK_END_4CC_CODE, writer);
			boundingDiscsChunkDataSize = writer.GetAppendedSize() - boundingDiscsChunkStartLocation;
			numChunksSaved++;
		}
	}

	// Tiled Bit Regions Chunk
	if (saveBitBuckets)
	{
		convexScene->GenerateBitMasksForAllPolys();

		tiledBitRegionsChunkStartLocation = writer.GetAppendedSize();
		Append4ccCodeToWriter(CONVEX_CHUNK_4CC_CODE, writer);
		writer.AppendByte((uint8_t)ChunkType::TILED_BIT_REGIONS);
		writer.AppendByte(endianModeCode);
		int payloadLocation = writer.GetAppendedSize();
		uint32_t payloadSize = 0;
		writer.AppendUint32(0x00); // payload size will go here
		writer.AppendVec2(convexScene->m_sceneBounds.m_mins);
		payloadSize += sizeof(Vec2);
		writer.AppendVec2(convexScene->m_sceneBounds.m_maxs);
		payloadSize += sizeof(Vec2);
		writer.AppendUShort((uint16_t)convexScene->m_currentNumPolys);
		payloadSize += sizeof(unsigned short);
		for (int polyIndex = 0; polyIndex < (int)convexScene->m_currentNumPolys; polyIndex++)
		{
			writer.AppendUint64(convexScene->m_bitBucketMasks[polyIndex]);
			payloadSize += sizeof(uint64_t);
		}
		writer.OverwriteUint32AtPosition(payloadSize, payloadLocation);
		Append4ccCodeToWriter(CONVEX_CHUNK_END_4CC_CODE, writer);
		tiledBitRegionsChunkDataSize = writer.GetAppendedSize() - tiledBitRegionsChunkStartLocation;
		numChunksSaved++;
	}

	// #ToDo Save any unknown chunks as they were loaded if the scene wasn't modified
	for (int unknownChunkIndex = 0; unknownChunkIndex < (int)convexScene->m_unknownFileChunksLoaded.size(); unknownChunkIndex++)
	{
		GHCSFileChunk& chunk = convexScene->m_unknownFileChunksLoaded[unknownChunkIndex];
		chunk.m_startIndexUsedForSave = writer.GetAppendedSize();
		Append4ccCodeToWriter(CONVEX_CHUNK_4CC_CODE, writer);
		writer.AppendByte((uint8_t)chunk.m_type);
		writer.AppendByte((uint8_t)chunk.m_endianness);
		writer.AppendUint32(chunk.m_payloadSize);
		for (uint32_t byteIndex = 0; byteIndex < chunk.m_payloadSize; byteIndex++)
		{
			writer.AppendByte(chunk.m_payload[byteIndex]);
		}
		Append4ccCodeToWriter(CONVEX_CHUNK_END_4CC_CODE, writer);
		chunk.m_totalSizeIncludingHeaderAndFooter = writer.GetAppendedSize() - chunk.m_startIndexUsedForSave;
	}

	// ToC
	{
		uint32_t tocLocation = writer.GetAppendedSize();
		writer.OverwriteUint32AtPosition(tocLocation, tocLocationInHeader);
		Append4ccCodeToWriter(CONVEX_SCENE_TOC_4CC_CODE, writer);
		writer.AppendByte(numChunksSaved);

		// Scene Info Chunk
		writer.AppendByte((uint8_t)ChunkType::SCENE_INFO);
		writer.AppendUint32(sceneInfoChunkStartLocation);
		writer.AppendUint32(sceneInfoChunkDataSize);

		// Convex Polys Chunk
		writer.AppendByte((uint8_t)ChunkType::CONVEX_POLYS);
		writer.AppendUint32(convexPolysChunkStartLocation);
		writer.AppendUint32(convexPolysChunkDataSize);

		// Convex Hulls Chunk
		if (saveConvexHulls)
		{
			writer.AppendByte((uint8_t)ChunkType::CONVEX_HULLS);
			writer.AppendUint32(convexHullsChunkStartLocation);
			writer.AppendUint32(convexHullsChunkDataSize);
		}

		// Bounding Discs Chunk
		if (saveBoundingDiscs)
		{
			writer.AppendByte((uint8_t)ChunkType::BOUNDING_DISCS);
			writer.AppendUint32(boundingDiscsChunkStartLocation);
			writer.AppendUint32(boundingDiscsChunkDataSize);
		}

		// Tiled Bit Regions Chunk
		if (saveBitBuckets)
		{
			writer.AppendByte((uint8_t)ChunkType::TILED_BIT_REGIONS);
			writer.AppendUint32(tiledBitRegionsChunkStartLocation);
			writer.AppendUint32(tiledBitRegionsChunkDataSize);
		}

		for (int unknownChunkIndex = 0; unknownChunkIndex < (int)convexScene->m_unknownFileChunksLoaded.size(); unknownChunkIndex++)
		{
			GHCSFileChunk& chunk = convexScene->m_unknownFileChunksLoaded[unknownChunkIndex];
			writer.AppendByte((uint8_t)chunk.m_type);
			writer.AppendUint32(chunk.m_startIndexUsedForSave);
			writer.AppendUint32(chunk.m_totalSizeIncludingHeaderAndFooter);
		}

		Append4ccCodeToWriter(CONVEX_SCENE_TOC_END_4CC_CODE, writer);
	}

	//---------------------------------------------------------------------------------------

	if (FileWriteBuffer(filePath, fileBuffer))
	{
		g_console->AddLine(DevConsole::INFO_MAJOR, Stringf("Successfully saved scene to %s", filePath.c_str()));
	}

	return false;
}

bool Command_LoadScene(EventArgs& args)
{
	std::string sceneName = args.GetValue("name", "");
	if (sceneName.empty())
	{
		g_console->AddLine(DevConsole::ERROR, "No scene name provided for load- skipping load!");
		return false;
	}

	std::string filePath = Stringf("Data/Scenes/%s.ghcs", sceneName.c_str());
	g_console->AddLine(DevConsole::INFO_MINOR, Stringf("Loading file %s", filePath.c_str()));
	
	std::vector<uint8_t> bufferToRead;
	FileReadToBuffer(bufferToRead, filePath);
	BufferParser parser(bufferToRead);

	Game* game = g_app->m_game;
	VisualTestConvexScene* convexScene = dynamic_cast<VisualTestConvexScene*>(game);

	convexScene->m_unknownFileChunksLoaded.clear();
	convexScene->m_convexPolys.clear();
	convexScene->m_convexHulls.clear();
	convexScene->m_boundingDiscs.clear();
	convexScene->m_bitBucketMasks.clear();

	// Header
	char const* convexScene4ccCode = Parse4ccCodeFromParser(parser);
	if (strcmp(convexScene4ccCode, CONVEX_SCENE_4CC_CODE))
	{
		g_console->AddLine(DevConsole::ERROR, "Invalid 4CC code. Aborting load!");
		return false;
	}

	uint8_t fileCohortID = parser.ParseByte();
	if (fileCohortID != COHORT_ID)
	{
		g_console->AddLine(DevConsole::ERROR, "Invalid cohort ID. Aborting load!");
		return false;
	}

	uint8_t fileMajorVersion = parser.ParseByte();
	if (fileMajorVersion != MAJOR_VERSION)
	{
		g_console->AddLine(DevConsole::ERROR, "Invalid major version. Aborting load!");
		return false;
	}

	uint8_t fileMinorVersion = parser.ParseByte();
	if (fileMinorVersion != MINOR_VERSION)
	{
		g_console->AddLine(DevConsole::ERROR, "Invalid minor version. Aborting load!");
		return false;
	}

	uint8_t endianModeCode = parser.ParseByte();
	if (endianModeCode == 1)
	{
		parser.SetEndianMode(BufferEndian::LITTLE);
	}
	else if (endianModeCode == 2)
	{
		parser.SetEndianMode(BufferEndian::BIG);
	}
	else
	{
		g_console->AddLine(DevConsole::ERROR, "Unknown endian mode. Aborting load");
		return false;
	}

	uint32_t tocLocation = parser.ParseUint32();

	char const* headerEnd4ccCode = Parse4ccCodeFromParser(parser);
	if (strcmp(headerEnd4ccCode, CONVEX_HEADER_END_4CC_CODE))
	{
		g_console->AddLine(DevConsole::ERROR, "Could not find header end 4CC code. Aborting load!");
		return false;
	}

	if (parser.GetSeekPosition() != tocLocation)
	{
		{
			// Scene Info chunk

			char const* chunkStart4ccCode = Parse4ccCodeFromParser(parser);
			if (strcmp(chunkStart4ccCode, CONVEX_CHUNK_4CC_CODE))
			{
				g_console->AddLine(DevConsole::ERROR, "Could not find chunk 4CC code for SceneInfo chunk. Aborting load!");
				return false;
			}
			uint8_t chunkType = parser.ParseByte();
			if (chunkType != (uint8_t)ChunkType::SCENE_INFO)
			{
				g_console->AddLine("Invalid chunk type immediately after header. Scene Info chunk should be the first chunk! Aborting load...");
				return false;
			}
			uint8_t chunkEndianMode = parser.ParseByte();
			if (chunkEndianMode == (uint8_t)BufferEndian::LITTLE)
			{
				parser.SetEndianMode(BufferEndian::LITTLE);
			}
			else if (chunkEndianMode == (uint8_t)BufferEndian::BIG)
			{
				parser.SetEndianMode(BufferEndian::BIG);
			}
			else
			{
				g_console->AddLine("Unknown endian mode for Scene Info chunk. Only little endian and big endian are supported! Aborting load...");
				return false;
			}
			[[maybe_unused]] uint32_t chunkDataSize = parser.ParseUint32();

			Vec2 worldBoundsMins = parser.ParseVec2();
			Vec2 worldBoundsMaxs = parser.ParseVec2();
			float worldBoundsAspect = (worldBoundsMaxs - worldBoundsMins).x / (worldBoundsMaxs - worldBoundsMins).y;
			convexScene->m_sceneBounds = AABB2(worldBoundsMins, worldBoundsMaxs);

			if (worldBoundsAspect < g_window->GetAspect())
			{
				// Need pillarbox borders
				convexScene->m_worldBounds.m_mins.y = worldBoundsMins.y;
				convexScene->m_worldBounds.m_maxs.y = worldBoundsMaxs.y;

				convexScene->m_worldBounds.m_mins.x = worldBoundsMins.x - convexScene->m_sceneBounds.GetDimensions().x * 0.5f;
				convexScene->m_worldBounds.m_maxs.x = convexScene->m_worldBounds.m_mins.x + (worldBoundsMaxs.y - worldBoundsMins.y) * g_window->GetAspect();
			}
			else if (worldBoundsAspect > g_window->GetAspect())
			{
				// Need letterbox borders
				convexScene->m_worldBounds.m_mins.x = worldBoundsMins.x;
				convexScene->m_worldBounds.m_maxs.x = worldBoundsMaxs.x;

				convexScene->m_worldBounds.m_mins.y = worldBoundsMins.y - convexScene->m_sceneBounds.GetDimensions().y * 0.5f;
				convexScene->m_worldBounds.m_maxs.y = convexScene->m_worldBounds.m_mins.y + (worldBoundsMaxs.x - worldBoundsMins.x) / g_window->GetAspect();
			}
			else
			{
				// Remove all borders, aspects are exactly equal
				convexScene->m_worldBounds = AABB2(worldBoundsMins, worldBoundsMaxs);
			}

			convexScene->m_currentNumPolys = parser.ParseUShort();
			char const* chunkEnd4ccCode = Parse4ccCodeFromParser(parser);
			if (strcmp(chunkEnd4ccCode, CONVEX_CHUNK_END_4CC_CODE))
			{
				g_console->AddLine(DevConsole::ERROR, "Could not find chunk end 4CC code for Scene Info chunk. Aborting load!");
				return false;
			}
		}


		{
			// Convex Polys chunk

			char const* chunkStart4ccCode = Parse4ccCodeFromParser(parser);
			if (strcmp(chunkStart4ccCode, CONVEX_CHUNK_4CC_CODE))
			{
				g_console->AddLine(DevConsole::ERROR, "Could not find chunk 4CC code for ConvexPolys chunk. Aborting load!");
				return false;
			}
			uint8_t chunkType = parser.ParseByte();
			if (chunkType != (uint8_t)ChunkType::CONVEX_POLYS)
			{
				g_console->AddLine("Invalid chunk type immediately after Scene Info chunk. Expected ConvexPolys chunk! Aborting load...");
				return false;
			}
			uint8_t chunkEndianMode = parser.ParseByte();
			if (chunkEndianMode == (uint8_t)BufferEndian::LITTLE)
			{
				parser.SetEndianMode(BufferEndian::LITTLE);
			}
			else if (chunkEndianMode == (uint8_t)BufferEndian::BIG)
			{
				parser.SetEndianMode(BufferEndian::BIG);
			}
			else
			{
				g_console->AddLine("Unknown endian mode for ConvexPolys chunk. Only little endian and big endian are supported! Aborting load...");
				return false;
			}
			[[maybe_unused]] uint32_t chunkDataSize = parser.ParseUint32();

			int numPolysInConvexPolysChunk = parser.ParseUShort();
			if (numPolysInConvexPolysChunk != convexScene->m_currentNumPolys)
			{
				g_console->AddLine("Mismatch in number of polys in SceneInfo chunk and ConvexPolys chunk. Number of polys in each should be the same! Aborting load...");
				return false;
			}

			for (int polyIndex = 0; polyIndex < numPolysInConvexPolysChunk; polyIndex++)
			{
				int numVertexes = parser.ParseByte();
				std::vector<Vec2> tempPolyVertexes;
				for (int vertexIndex = 0; vertexIndex < numVertexes; vertexIndex++)
				{
					tempPolyVertexes.push_back(parser.ParseVec2());
				}
				convexScene->m_convexPolys.push_back(ConvexPoly2(tempPolyVertexes));
			}

			char const* chunkEnd4ccCode = Parse4ccCodeFromParser(parser);
			if (strcmp(chunkEnd4ccCode, CONVEX_CHUNK_END_4CC_CODE))
			{
				g_console->AddLine(DevConsole::ERROR, "Could not find chunk end 4CC code for ConvexPolys chunk. Aborting load!");
				return false;
			}
		}
	}
	else
	{
		convexScene->m_currentNumPolys = 0;
	}

	while (parser.GetSeekPosition() != tocLocation)
	{
		if (!LoadChunkFromParser(parser))
		{
			return false;
		}
	}
	//---------------------------------------------------------------------------------------

	g_console->AddLine(DevConsole::INFO_MAJOR, Stringf("Successfully loaded file %s", filePath.c_str()));
	g_console->AddLine(DevConsole::INFO_MINOR, Stringf("Loaded %d convex polys", convexScene->m_currentNumPolys));
	if (convexScene->m_currentNumPolys > 0 && convexScene->m_convexHulls.empty())
	{
		convexScene->GenerateHullsForAllPolys();
		g_console->AddLine(DevConsole::WARNING, Stringf("No convex hulls loaded. Hulls were generated after loading polys."));
	}
	else if (!convexScene->m_convexHulls.empty())
	{
		g_console->AddLine(DevConsole::INFO_MINOR, Stringf("Loaded convex hulls for all polys."));
	}
	if (convexScene->m_currentNumPolys > 0 && convexScene->m_boundingDiscs.empty())
	{
		convexScene->GenerateBoundingDiscsForAllPolys();
		g_console->AddLine(DevConsole::WARNING, Stringf("No bounding discs loaded. Bounding discs were generated after loading polys and may not be the best fits."));
	}
	else if (!convexScene->m_boundingDiscs.empty())
	{
		g_console->AddLine(DevConsole::INFO_MINOR, Stringf("Loaded bounding discs for all polys."));
	}
	if (convexScene->m_currentNumPolys > 0 && convexScene->m_bitBucketMasks.empty())
	{
		g_console->AddLine(DevConsole::WARNING, Stringf("No tiled bit regions loaded. Tiled bit regions will be generated when testing raycasts."));
	}
	else if (!convexScene->m_bitBucketMasks.empty())
	{
		g_console->AddLine(DevConsole::INFO_MINOR, Stringf("Loaded tiled bit region masks for all polys."));
	}

	if (!convexScene->m_unknownFileChunksLoaded.empty())
	{
		g_console->AddLine(DevConsole::INFO_MINOR, Stringf("Loaded %d unknown chunks. Chunks will be written to save file if the scene is not modified.", (int)convexScene->m_unknownFileChunksLoaded.size()));
	}

	return false;
}

bool LoadChunkFromParser(BufferParser& parser)
{
	Game* game = g_app->m_game;
	VisualTestConvexScene* convexScene = dynamic_cast<VisualTestConvexScene*>(game);

	GHCSFileChunk chunk;
	char const* chunk4ccCode = Parse4ccCodeFromParser(parser);
	if (strcmp(chunk4ccCode, CONVEX_CHUNK_4CC_CODE))
	{
		g_console->AddLine(DevConsole::ERROR, "Chunk 4CC header not found for optional chunk. Aborting load!");
		return false;
	}

	chunk.m_type = ChunkType(parser.ParseByte());
	uint8_t chunkEndianCode = parser.ParseByte();
	if (chunkEndianCode == 1 || chunkEndianCode == 2)
	{
		chunk.m_endianness = BufferEndian(chunkEndianCode);
		parser.SetEndianMode((BufferEndian)chunk.m_endianness);
	}
	else
	{
		g_console->AddLine(DevConsole::ERROR, "Unknown endian mode for optional chunk. Only little and big endian are supported! Aborting load...");
	}
	chunk.m_payloadSize = parser.ParseUint32();

	if (chunk.m_type == ChunkType::CONVEX_HULLS)
	{
		// #ToDo Handle loading convex hulls here
		uint16_t numHulls = parser.ParseUShort();
		if (numHulls != convexScene->m_currentNumPolys)
		{
			g_console->AddLine(DevConsole::ERROR, "Number of convex hulls specified in ConvexHulls chunk does not match number of polys specified in header. Aborting load!");
			return false;
		}

		for (uint16_t hullIndex = 0; hullIndex < numHulls; hullIndex++)
		{
			uint8_t numPlanesInHull = parser.ParseByte();
			std::vector<Plane2> tempPlanes;
			for (int planeIndex = 0; planeIndex < numPlanesInHull; planeIndex++)
			{
				Plane2 plane;
				plane.m_normal = parser.ParseVec2();
				plane.m_distanceFromOriginAlongNormal = parser.ParseFloat();
				tempPlanes.push_back(plane);
			}
			convexScene->m_convexHulls.push_back(ConvexHull2(tempPlanes));
		}
	}
	else if (chunk.m_type == ChunkType::BOUNDING_DISCS)
	{
		// #ToDo Handle loading bounding discs here
		uint16_t numDiscs = parser.ParseUShort();
		if (numDiscs != convexScene->m_currentNumPolys)
		{
			g_console->AddLine(DevConsole::ERROR, "Number of bounding discs specified in BoundingDiscs chunk does not match number of polys specified in header. Aborting load!");
			g_console->AddLine(DevConsole::INFO_MINOR, Stringf("Convex Polys according to header = %d, Bounding discs according to chunk = %d", numDiscs, convexScene->m_currentNumPolys));
			return false;
		}

		for (uint16_t discIndex = 0; discIndex < numDiscs; discIndex++)
		{
			BoundingDisc disc;
			disc.m_center = parser.ParseVec2();
			disc.m_radius = parser.ParseFloat();
			convexScene->m_boundingDiscs.push_back(disc);
		}
	}
	else if (chunk.m_type == ChunkType::TILED_BIT_REGIONS)
	{
		// #ToDo Handle loading tiled bit regions here
		Vec2 worldBoundsMins = parser.ParseVec2();
		Vec2 worldBoundsMaxs = parser.ParseVec2();

		uint16_t numPolys = parser.ParseUShort();
		if (numPolys != convexScene->m_currentNumPolys)
		{
			g_console->AddLine(DevConsole::ERROR, "Number of polys specified in TiledBitRegions chunk does not match number of polys specified in header. Aborting load!");
			return false;
		}
		for (uint16_t polyIndex = 0; polyIndex < numPolys; polyIndex++)
		{
			convexScene->m_bitBucketMasks.push_back(parser.ParseUint64());
		}

		if (worldBoundsMins != convexScene->m_sceneBounds.m_mins || worldBoundsMaxs != convexScene->m_sceneBounds.m_maxs)
		{
			g_console->AddLine(DevConsole::WARNING, "World bounds used for TiledBitRegions varies from world bounds specified in SceneInfo. Tiled bit regions will be regenerated but loading will continue.");
			convexScene->m_needToRegenerateBitMasks = true;
		}
		else
		{
			convexScene->m_needToRegenerateBitMasks = true;
		}
	}
	else
	{
		// All other chunks are unknown
		for (uint32_t byteIndex = 0; byteIndex < chunk.m_payloadSize; byteIndex++)
		{
			chunk.m_payload.push_back(parser.ParseByte());
		}

		convexScene->m_unknownFileChunksLoaded.push_back(chunk);
	}

	char const* chunkEnd4ccCode = Parse4ccCodeFromParser(parser);
	if (strcmp(chunkEnd4ccCode, CONVEX_CHUNK_END_4CC_CODE))
	{
		g_console->AddLine(DevConsole::ERROR, "Chunk 4CC footer not found for optional chunk. Aborting load!");
		return false;
	}

	return true;
}
