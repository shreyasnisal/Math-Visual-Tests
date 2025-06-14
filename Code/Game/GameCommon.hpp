#pragma once

#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/SimpleTriangleFont.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Renderer.hpp"

class App;

extern App*								g_app;
extern RandomNumberGenerator*			g_RNG;
extern Renderer*						g_renderer;
extern Window*							g_window;
extern BitmapFont*						g_squirrelFont;

constexpr float WORLD_SIZE_X			= 200.f;
constexpr float WORLD_SIZE_Y			= 100.f;
constexpr float SCREEN_SIZE_X			= 1600.f;
constexpr float SCREEN_SIZE_Y			= 800.f;
constexpr float ASPECT					= SCREEN_SIZE_X / SCREEN_SIZE_Y;

extern char const* CONVEX_SCENE_4CC_CODE;
extern char const* CONVEX_HEADER_END_4CC_CODE;
extern char const* CONVEX_CHUNK_4CC_CODE;
extern char const* CONVEX_CHUNK_END_4CC_CODE;
extern char const* CONVEX_SCENE_TOC_4CC_CODE;
extern char const* CONVEX_SCENE_TOC_END_4CC_CODE;
constexpr uint8_t COHORT_ID = 33;
constexpr uint8_t MAJOR_VERSION = 1;
constexpr uint8_t MINOR_VERSION = 1;

enum class ChunkType : uint8_t
{
	INVALID = 0x00,
	SCENE_INFO = 0x01,
	CONVEX_POLYS = 0x02,

	CONVEX_HULLS = 0x80,
	BOUNDING_DISCS = 0x81,
	BOUNDING_AABBS = 0x82,
	BVH_AABB2_TREE = 0x83,
	BVH_OBB2_TREE = 0x84,
	BVH_CONVEX_HULL_TREE = 0x85,
	ASYMMETRIC_QUADTREE = 0x86,
	SYMMETRIC_QUADTREE = 0x87,
	TILED_BIT_REGIONS = 0x88,
	COLUMN_ROW_BIT_REGIONS = 0x89,
	BVH_DISC2_TREE = 0x8A,
	BSP2_TREE = 0x8B,
	BVH_COMPOSITE_TREE = 0x8C,
	BVH_CONVEX_POLY_TREE = 0x8D,
};
