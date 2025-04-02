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
