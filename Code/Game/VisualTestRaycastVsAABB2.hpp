#pragma once

#include "Game/Game.hpp"

class VisualTestRaycastVsAABB2 : public Game
{
public:
	~VisualTestRaycastVsAABB2() = default;
	VisualTestRaycastVsAABB2();
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void Randomize() override;

public:
	std::vector<AABB2> m_aabbs;
	Vec2 m_raycastStartPosition;
	Vec2 m_raycastEndPosition;
};
