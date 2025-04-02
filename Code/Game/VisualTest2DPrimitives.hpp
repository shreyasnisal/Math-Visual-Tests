#pragma once

#include "Game/Game.hpp"

class VisualTest2DPrimitives : public Game
{
public:
	~VisualTest2DPrimitives() = default;
	VisualTest2DPrimitives();
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void Randomize() override;

public:
		Vec2 m_referencePointPosition = Vec2(WORLD_SIZE_X, WORLD_SIZE_Y) * 0.5f;
		Vec2 m_discCenter;
		float m_discRadius = 0.f;
		Vec2 m_lineSegmentStart;
		Vec2 m_lineSegmentEnd;
		Vec2 m_linePointA;
		Vec2 m_linePointB;
		AABB2 m_axisAlignedBox;
		OBB2 m_orientedBox;
		Vec2 m_capsuleBoneStart;
		Vec2 m_capsuleBoneEnd;
		float m_capsuleRadius = 0.f;
		Vec2 m_orientedSectorTipPosition;
		float m_orientedSectorRadius = 0.f;
		float m_orientedSectorOrientationDegrees = 0.f;
		float m_orientedSectorApertureDegrees = 0.f;
		Vec2 m_directedSectorTipPosition;
		float m_directedSectorRadius = 0.f;
		Vec2 m_directedSectorForwardNormal;
		float m_directedSectorApertureDegrees = 0.f;
};