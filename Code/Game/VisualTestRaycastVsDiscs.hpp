#pragma once

#include "Game/Game.hpp"

struct Disc2D
{
public:
	float m_radius = 0.0f;
	Vec2 m_center;
};

class VisualTestRaycastVsDiscs : public Game
{
public:
	~VisualTestRaycastVsDiscs() = default;
	VisualTestRaycastVsDiscs();
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void Randomize() override;

public:
	std::vector<Disc2D> m_discs;
	Vec2 m_raycastStartPosition;
	Vec2 m_raycastEndPosition;
};
