#pragma once

#include "Game/Game.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Stopwatch.hpp"

constexpr int NUM_BUMPERS_PER_TYPE = 10;

struct PachinkoBall;

enum class BumperType
{
	INVALID = -1,

	CAPSULE,
	DISC,
	OBB2,

	COUNT
};

struct PachinkoBumper
{
public:
	BumperType m_type = BumperType::INVALID;
	Rgba8 m_color = Rgba8::WHITE;
	Vec2 m_center = Vec2::ZERO;
	Vec2 m_halfDimensions = Vec2::ZERO;
	Vec2 m_iBasis = Vec2::ZERO;
	float m_radius = 0.f;
	Vec2 m_start = Vec2::ZERO;
	Vec2 m_end = Vec2::ZERO;
	Vec2 m_boundingDiscCenter = Vec2::ZERO;
	float m_boundingDiscRadius = 0.f;

	float m_elasticity = 0.f;

public:
	~PachinkoBumper() = default;
	PachinkoBumper() = default;
	PachinkoBumper(float elasticity);
	bool BounceBall(PachinkoBall& ball) const;
	void AddVerts(std::vector<Vertex_PCU>& verts);
};

struct PachinkoBall
{
public:
	Rgba8 m_color = Rgba8::WHITE;
	Vec2 m_center = Vec2::ZERO;
	float m_radius = 0.f;
	Vec2 m_velocity = Vec2::ZERO;
	float m_elasticity = 0.f;

public:
	~PachinkoBall() = default;
	PachinkoBall();
	void AddVerts(std::vector<Vertex_PCU>& verts);
};

class VisualTestPachinkoMachine : public Game
{
public:
	~VisualTestPachinkoMachine();
	VisualTestPachinkoMachine();

	virtual void Update(float deltaSeconds) override;
	
	void HandleInput();
	void LaunchBall();

	void UpdatePhysics(float deltaSeconds);
	void UpdateBalls(float deltaSeconds);
	void PushBallsOutOfEachOther();
	void PushBallsOutOfBumpers();
	void PushBallsIntoWorldBounds();
	
	virtual void Randomize() override;

	virtual void Render() const override;

public:
	Clock* m_gameClock = nullptr;
	Stopwatch m_physicsTimer;
	PachinkoBumper* m_bumpers[(int)BumperType::COUNT * NUM_BUMPERS_PER_TYPE];
	std::vector<PachinkoBall*> m_balls;
	Vec2 m_launchPosition = Vec2::ZERO;
	Vec2 m_launchRayEnd = Vec2::ZERO;
	bool m_isFloorFallThrough = false;
	float m_physicsTimeStepSize = 0.005f;
};
