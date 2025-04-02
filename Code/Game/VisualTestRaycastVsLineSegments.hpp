#pragma once

#include "Game/Game.hpp"

struct LineSegment2D
{
public:
	Vec2 m_start;
	Vec2 m_end;
};

class VisualTestRaycastVsLineSegments : public Game
{
public:
	~VisualTestRaycastVsLineSegments() = default;
	VisualTestRaycastVsLineSegments();
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void Randomize() override;

public:
	std::vector<LineSegment2D> m_lineSegments;
	Vec2 m_raycastStartPosition;
	Vec2 m_raycastEndPosition;
};
