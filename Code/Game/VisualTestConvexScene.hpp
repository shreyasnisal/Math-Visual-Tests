#pragma once

#include "Game/Game.hpp"

#include "Engine/Math/ConvexPoly2.hpp"
#include "Engine/Math/ConvexHull2.hpp"

#include <vector>

struct RaycastResult2D;


struct BoundingDisc
{
public:
	Vec2 m_center = Vec2::ZERO;
	float m_radius = 0.f;
	bool m_visible = false;
};

enum class OptimizationMode
{
	NONE,
	NARROW_PHASE_BOUNDING_DISC_ONLY,
	BROAD_PHASE_BSP_ONLY,
	NARROW_AND_BROAD_PHASE,
	NUM
};

std::string GetOptimizationModeStr(OptimizationMode optimizationMode);

class VisualTestConvexScene : public Game
{
public:
	virtual ~VisualTestConvexScene() = default;
	VisualTestConvexScene();
	virtual void Update(float deltaSeconds);
	virtual void Render() const;
	virtual void Randomize();

	void HandleInput();
	void RotatePolyAtIndexAroundPointByDegrees(int polyIndex, Vec2 const& point, float degrees);
	void ScalePolyAtIndexAroundPointByFactor(int polyIndex, Vec2 const& point, float scalingFactor);

	void GenerateHullsForAllPolys();
	void RegenerateHullForForPolyAtIndex(int polyIndex);

	RaycastResult2D RaycastVsConvexHull2_WithDebugDrawWhenForSimpleHull(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDistance, ConvexHull2 const& convexHull, std::vector<Vertex_PCU>& verts) const;

	void GenerateRandomRaycasts();
	void PerformAllTestRaycasts();

public:
	static constexpr int NUM_INITIAL_POLYS = 8;
	static constexpr int NUM_MAX_POLYS = 1024;

	static constexpr int NUM_INITIAL_RAYCASTS = 1024;
	static constexpr int NUM_MAX_RAYCASTS = 2048;
	
	static constexpr float BOUNDING_DISC_MIN_RADIUS = 5.f;
	static constexpr float BOUNDING_DISC_MAX_RADIUS = 20.f;
	
	static constexpr float MIN_THETA_INCREMENT_FOR_POLY_VERTEX = 15.f;
	static constexpr float MAX_THETA_INCREMENT_FOR_POLY_VERTEX = 120.f;

	static constexpr float RAY_MIN_LENGTH = 10.f;
	static constexpr float RAY_MAX_LENGTH = 100.f;

	Clock* m_gameClock = nullptr;

	std::vector<ConvexPoly2> m_convexPolys;
	std::vector<ConvexHull2> m_convexHulls;
	std::vector<BoundingDisc> m_boundingDiscs;

	int m_currentNumPolys = NUM_INITIAL_POLYS;

	bool m_drawWithTranslucentFill = false;

	int m_hoveredConvexPolyIndex = -1;
	int m_selectedConvexPolyIndex = -1;
	Vec2 m_selectedPolyOffsetFromCursorPosition = Vec2::ZERO;
	std::vector<Vec2> m_vertexOffsetsFromCursorPosition;

	bool m_isMovingRaycast = false;
	Vec2 m_visibleRaycastStart = Vec2::ZERO;
	Vec2 m_visibleRaycastEnd = Vec2::ZERO;

	int m_currentNumRaycasts = NUM_INITIAL_RAYCASTS;

	OptimizationMode m_currentOptimizationMode = OptimizationMode::NONE;

	std::vector<Vec2> m_rayStartPositions;
	std::vector<Vec2> m_rayFwdNormals;
	std::vector<float> m_rayMaxDistances;

	double m_totalRaycastTimeMs = -1.f;
	float m_averageRaycastImpactDistance = -1.f;
	int m_raycastsPerformedInLastTest = 0;
};
