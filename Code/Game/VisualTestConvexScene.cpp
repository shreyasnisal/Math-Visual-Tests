#include "Game/VisualTestConvexScene.hpp"

#include "Engine/Core/Time.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"


VisualTestConvexScene::VisualTestConvexScene()
{
	g_input->SetCursorMode(false, false);

	m_worldCamera.SetOrthoView(Vec2::ZERO, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	m_gameClock = new Clock(Clock::GetSystemClock());

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
	DebugAddMessage(Stringf("Num Polys = %d; Num Raycasts = %d; Optimization = %s;", m_currentNumPolys, m_currentNumRaycasts, GetOptimizationModeStr(m_currentOptimizationMode).c_str()), 0.f, Rgba8::WHITE, Rgba8::WHITE);
	DebugAddMessage(Stringf("F1 = Toggle bounding disc debug draw (per polygon); F9 = Cycle optimization modes; T = Fire raycasts"), 0.f, Rgba8::CYAN, Rgba8::CYAN);
	DebugAddMessage(Stringf("F8 = Reset; LMB/RMB = Move raycst start/end; LMB = Drag poly; A/D = Rotate; W/S = Scale; Q/E = Change num polygons; Z/C = Change num raycasts"), 0.f, Rgba8::CYAN, Rgba8::CYAN);
	DebugAddMessage("Mode [F6/F7 = Prev/Next]: Convex Scene (2D)", 0.f, Rgba8::YELLOW, Rgba8::YELLOW);
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
		AddOutlineVertsForConvexPoly2(vertexes, m_convexPolys[polyIndex], POLY_OUTLINE_THICKNESS, outlineColor);
	}

	for (int polyIndex = 0; polyIndex < m_convexPolys.size(); polyIndex++)
	{
		AddVertsForConvexPoly2(vertexes, m_convexPolys[polyIndex], fillColor);
	}

	for (int polyIndex = 0; polyIndex < m_convexPolys.size(); polyIndex++)
	{
		if (m_boundingDiscs[polyIndex].m_visible)
		{
			AddVertsForRing2D(vertexes, m_boundingDiscs[polyIndex].m_center, m_boundingDiscs[polyIndex].m_radius, BOUNDING_DISC_THICKNESS, Rgba8::MAGENTA);
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
			AddVertsForLineSegment2D(vertexes, planeCenter - lineDirection * 1000.f, planeCenter + lineDirection * 1000.f, 0.1f, Rgba8::MAGENTA);
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
		AddVertsForArrow2D(vertexes, m_visibleRaycastStart, m_visibleRaycastEnd, 1.f, 0.2f, Rgba8::GRAY);
		AddVertsForArrow2D(vertexes, m_visibleRaycastStart, closestRaycastResult.m_impactPosition, 1.f, 0.2f, Rgba8::GREEN);
		AddVertsForArrow2D(vertexes, closestRaycastResult.m_impactPosition, closestRaycastResult.m_impactPosition + closestRaycastResult.m_impactNormal * 10.f, 1.f, 0.2f, Rgba8::YELLOW);
		AddVertsForDisc2D(vertexes, closestRaycastResult.m_impactPosition, 0.4f, Rgba8::WHITE);
	}
	else
	{
		AddVertsForArrow2D(vertexes, m_visibleRaycastStart, m_visibleRaycastEnd, 1.f, 0.2f, Rgba8::RED);
	}

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
		boundingDisc.m_radius = g_RNG->RollRandomFloatInRange(BOUNDING_DISC_MIN_RADIUS, BOUNDING_DISC_MAX_RADIUS);
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
	Vec2 cursorWorldPosition;
	cursorWorldPosition.x = RangeMap(cursorNormalizedPosition.x, 0.f, 1.f, 0.f, WORLD_SIZE_X);
	cursorWorldPosition.y = RangeMap(cursorNormalizedPosition.y, 0.f, 1.f, 0.f, WORLD_SIZE_Y);

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
		m_boundingDiscs[m_selectedConvexPolyIndex].m_center = cursorWorldPosition + m_selectedPolyOffsetFromCursorPosition;
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
			m_selectedPolyOffsetFromCursorPosition = m_boundingDiscs[m_selectedConvexPolyIndex].m_center - cursorWorldPosition;

			std::vector<Vec2> selectedPolyVertexes = m_convexPolys[m_selectedConvexPolyIndex].GetVertexes();
			for (int vertexIndex = 0; vertexIndex < selectedPolyVertexes.size(); vertexIndex++)
			{
				m_vertexOffsetsFromCursorPosition.push_back(selectedPolyVertexes[vertexIndex] - cursorWorldPosition);
			}
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
		m_boundingDiscs[m_hoveredConvexPolyIndex].m_visible = !m_boundingDiscs[m_hoveredConvexPolyIndex].m_visible;
	}
	if (g_input->WasKeyJustPressed(KEYCODE_F2))
	{
		m_drawWithTranslucentFill = !m_drawWithTranslucentFill;
	}
	if (g_input->WasKeyJustPressed(KEYCODE_F9))
	{
		m_currentOptimizationMode = OptimizationMode(((int)m_currentOptimizationMode + 1) % (int)OptimizationMode::NUM);
	}
	if (g_input->WasKeyJustPressed('E'))
	{
		if (m_currentNumPolys < NUM_MAX_POLYS)
		{
			m_currentNumPolys *= 2;
			Randomize();
		}
	}
	if (g_input->WasKeyJustPressed('Q'))
	{
		if (m_currentNumPolys > 1)
		{
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

	Vec2 displacementPointToBoundingDiscCenter = m_boundingDiscs[polyIndex].m_center - point;
	m_boundingDiscs[polyIndex].m_center = point + displacementPointToBoundingDiscCenter.GetRotatedDegrees(degrees);
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

	Vec2 displacementPointToBoundingDiscCenter = m_boundingDiscs[polyIndex].m_center - point;
	m_boundingDiscs[polyIndex].m_center = point + displacementPointToBoundingDiscCenter * scalingFactor;
	m_boundingDiscs[polyIndex].m_radius *= scalingFactor;
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

void VisualTestConvexScene::GenerateRandomRaycasts()
{
	m_rayStartPositions.clear();
	m_rayFwdNormals.clear();
	m_rayMaxDistances.clear();

	for (int rayIndex = 0; rayIndex < m_currentNumRaycasts; rayIndex++)
	{
		m_rayStartPositions.push_back(g_RNG->RollRandomVec2InBox(AABB2(Vec2::ZERO, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y))));
		m_rayFwdNormals.push_back(g_RNG->RollRandomVec2InRadius(Vec2::ZERO, 1.f));
		m_rayMaxDistances.push_back(g_RNG->RollRandomFloatInRange(RAY_MIN_LENGTH, RAY_MAX_LENGTH));
	}
}

// #ToDo: Something wrong with the average raycast distance
void VisualTestConvexScene::PerformAllTestRaycasts()
{
	m_raycastsPerformedInLastTest = m_currentNumRaycasts;
	float totalImpactDistance = 0.f;
	double raycastStartTimeSeconds = GetCurrentTimeSeconds();
	for (int rayIndex = 0; rayIndex < m_currentNumRaycasts; rayIndex++)
	{
		for (int polyIndex = 0; polyIndex < m_currentNumPolys; polyIndex++)
		{
			if (m_currentOptimizationMode == OptimizationMode::NARROW_PHASE_BOUNDING_DISC_ONLY || m_currentOptimizationMode == OptimizationMode::NARROW_AND_BROAD_PHASE)
			{
				RaycastResult2D raycastVsDiscResult = RaycastVsDisc2D(m_rayStartPositions[rayIndex], m_rayFwdNormals[rayIndex], m_rayMaxDistances[rayIndex], m_boundingDiscs[polyIndex].m_center, m_boundingDiscs[polyIndex].m_radius);
				if (!raycastVsDiscResult.m_didImpact)
				{
					continue;
				}
			}

			RaycastResult2D raycastVsConvexHullResult = RaycastVsConvexHull2(m_rayStartPositions[rayIndex], m_rayFwdNormals[rayIndex], m_rayMaxDistances[rayIndex], m_convexHulls[polyIndex]);
			if (raycastVsConvexHullResult.m_didImpact)
			{
				totalImpactDistance += raycastVsConvexHullResult.m_impactDistance;
			}
			else
			{
				totalImpactDistance += m_rayMaxDistances[rayIndex];
			}
		}
	}
	double raycastEndTimeSeconds = GetCurrentTimeSeconds();
	m_totalRaycastTimeMs = (raycastEndTimeSeconds - raycastStartTimeSeconds) * 1000.f;
	m_averageRaycastImpactDistance = totalImpactDistance / (float)m_currentNumRaycasts;
}

std::string GetOptimizationModeStr(OptimizationMode optimizationMode)
{
	switch (optimizationMode)
	{
		case OptimizationMode::NONE:								return "None";									break;
		case OptimizationMode::NARROW_PHASE_BOUNDING_DISC_ONLY:		return "Narrow Phase Only (Bounding Disc)";		break;
		case OptimizationMode::BROAD_PHASE_BSP_ONLY:				return "Broad Phase Only (BSP)";				break;
		case OptimizationMode::NARROW_AND_BROAD_PHASE:				return "Narrow and Broad Phase";				break;
	}

	return "";
}
