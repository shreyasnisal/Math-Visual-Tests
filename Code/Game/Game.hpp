#pragma once

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Tile.hpp"


class		App;
class		Entity;

enum class VisualTestMode
{
	VISUAL_TEST_MODE_INVALID = -1,

	VISUAL_TEST_CONVEX_SCENE,
	VISUAL_TEST_MODE_PACHINKO_MACHINE,
	VISUAL_TEST_MODE_SPLINES,
	VISUAL_TEST_MODE_3D_GEOMETRY,
	VISUAL_TEST_MODE_RAYCAST_VS_AABB2,
	VISUAL_TEST_MODE_RAYCAST_VS_LINESEGMENTS,
	VISUAL_TEST_MODE_RAYCAST_VS_DISCS,
	VISUAL_TEST_MODE_2D_PRIMITIVES,
	VISUAL_TEST_MODE_RAYCAST_VS_TILES,

	VISUAL_TEST_MODE_NUM
};

enum class GameState
{
	PLAYING,
	PAUSED
};

class Game
{
public:
	virtual ~Game() = default;
	Game() = default;
	virtual void Update(float deltaSeconds) = 0;
	virtual void Render() const = 0;
	virtual void Randomize() = 0;

	static Game* CreateNewGameOfMode(VisualTestMode mode);

public:
	GameState m_gameState = GameState::PLAYING;
	Camera m_worldCamera = Camera();
	Camera m_screenCamera = Camera();
};