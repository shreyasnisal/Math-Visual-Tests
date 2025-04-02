#pragma once

#include "Game/Game.hpp"
#include "Game/Tile.hpp"

class TileHeatMap;

class VisualTestRaycastVsTiles : public Game
{
public:
	~VisualTestRaycastVsTiles() = default;
	VisualTestRaycastVsTiles();
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void Randomize() override;

public:
	TileHeatMap*		m_tileHeatmap = nullptr;
	Vec2				m_raycastStartPosition;
	Vec2				m_raycastEndPosition;
};
