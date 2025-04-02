#include "Game/VisualTestPachinkoMachine.hpp"

#include "Engine/Renderer/DebugRenderSystem.hpp"

constexpr float DISC_MIN_RADIUS = 2.f;
constexpr float DISC_MAX_RADIUS = 5.f;

constexpr float BOX_MIN_DIMENSION = 4.f;
constexpr float BOX_MAX_DIMENSION = 6.f;

constexpr float CAPSULE_MIN_LENGTH = 2.f;
constexpr float	CAPSULE_MAX_LENGTH = 5.f;
constexpr float CAPSULE_MIN_RADIUS = 2.f;
constexpr float CAPSULE_MAX_RADIUS = 5.f;

constexpr float BALL_MIN_RADIUS = 1.f;
constexpr float BALL_MAX_RADIUS = 3.f;

constexpr float BALL_ELASTICITY = 0.9f;
constexpr float WALLS_ELASTICITY = 0.6f;

PachinkoBumper::PachinkoBumper(float elasticity)
{
	m_elasticity = elasticity;
	float colorLerpFactor = RangeMap(elasticity, 0.1f, 0.9f, 0.f, 1.f);
	m_color = Interpolate(Rgba8::RED, Rgba8::LIME, colorLerpFactor);
}

bool PachinkoBumper::BounceBall(PachinkoBall& ball) const
{
	switch (m_type)
	{
		case BumperType::CAPSULE:
		{
			return BounceDiscOffFixedCapsule2D(ball.m_center, ball.m_radius, ball.m_velocity, ball.m_elasticity, m_start, m_end, m_radius, m_elasticity);
		}
		case BumperType::DISC:
		{
			 return BounceDiscOffFixedDisc2D(ball.m_center, ball.m_radius, ball.m_velocity, ball.m_elasticity, m_center, m_radius, m_elasticity);
		}
		case BumperType::OBB2:
		{
			return BounceDiscOffFixedOBB2(ball.m_center, ball.m_radius, ball.m_velocity, ball.m_elasticity, OBB2(m_center, m_iBasis, m_halfDimensions), m_elasticity);
		}
	}

	return false;
}

void PachinkoBumper::AddVerts(std::vector<Vertex_PCU>& verts)
{

	switch (m_type)
	{
		case BumperType::CAPSULE:
		{
			AddVertsForCapsule2D(verts, m_start, m_end, m_radius, m_color);
			break;
		}
		case BumperType::DISC:
		{
			AddVertsForDisc2D(verts, m_center, m_radius, m_color);
			break;
		}
		case BumperType::OBB2:
		{
			AddVertsForOBB2(verts, OBB2(m_center, m_iBasis, m_halfDimensions), m_color);
			break;
		}
	}
}

PachinkoBall::PachinkoBall()
{
	float colorToWhiteLerpFactor = g_RNG->RollRandomFloatInRange(0.f, 0.8f);
	m_color = Interpolate(Rgba8::DODGER_BLUE, Rgba8::WHITE, colorToWhiteLerpFactor);
}

void PachinkoBall::AddVerts(std::vector<Vertex_PCU>& verts)
{
	AddVertsForDisc2D(verts, m_center, m_radius, m_color);
}

VisualTestPachinkoMachine::~VisualTestPachinkoMachine()
{
	Clock::GetSystemClock().RemoveChild(m_gameClock);
	delete m_gameClock;
	m_gameClock = nullptr;
}

VisualTestPachinkoMachine::VisualTestPachinkoMachine()
{
	g_input->SetCursorMode(false, false);

	m_worldCamera.SetOrthoView(Vec2::ZERO, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

	m_gameClock = new Clock(Clock::GetSystemClock());
	m_physicsTimer = Stopwatch(m_gameClock, m_physicsTimeStepSize);

	m_balls.reserve(1000);

	Randomize();
}

void VisualTestPachinkoMachine::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	HandleInput();
	while (m_physicsTimer.DecrementDurationIfElapsed())
	{
		UpdatePhysics(m_physicsTimeStepSize);
	}

	DebugAddMessage(Stringf("F8 to Reset; LMB/RMB/WASD/IJKL/Arrow keys = move; T = slow; space/N = ball (%d); B = bottom wrap %s; timestep = %.2f ms; dt = %.1f ms", (int)m_balls.size(), (m_isFloorFallThrough ? "Off" : "On"), m_physicsTimeStepSize * 1000.f, m_gameClock->GetDeltaSeconds() * 1000.f / m_gameClock->GetTimeScale()), 0.f, Rgba8::CYAN, Rgba8::CYAN);
	DebugAddMessage("Mode [F6/F7 for Prev/Next]: Pachinko Machine (2D)", 0.f, Rgba8::YELLOW, Rgba8::YELLOW);
}

void VisualTestPachinkoMachine::HandleInput()
{
	//float deltaSeconds = m_gameClock.GetDeltaSeconds();
	constexpr float POINT_MOVEMENT_SPEED = 1.f;

	// Arrow keys
	if (g_input->IsKeyDown(KEYCODE_UPARROW))
	{
		m_launchPosition += Vec2::NORTH * POINT_MOVEMENT_SPEED;
		m_launchRayEnd += Vec2::NORTH * POINT_MOVEMENT_SPEED;
	}
	if (g_input->IsKeyDown(KEYCODE_DOWNARROW))
	{
		m_launchPosition += Vec2::SOUTH * POINT_MOVEMENT_SPEED;
		m_launchRayEnd += Vec2::SOUTH * POINT_MOVEMENT_SPEED;
	}
	if (g_input->IsKeyDown(KEYCODE_RIGHTARROW))
	{
		m_launchPosition += Vec2::EAST * POINT_MOVEMENT_SPEED;
		m_launchRayEnd += Vec2::EAST * POINT_MOVEMENT_SPEED;
	}
	if (g_input->IsKeyDown(KEYCODE_LEFTARROW))
	{
		m_launchPosition += Vec2::WEST * POINT_MOVEMENT_SPEED;
		m_launchRayEnd += Vec2::WEST * POINT_MOVEMENT_SPEED;
	}

	// WASD
	if (g_input->IsKeyDown('W'))
	{
		m_launchPosition += Vec2::NORTH * POINT_MOVEMENT_SPEED;
	}
	if (g_input->IsKeyDown('S'))
	{
		m_launchPosition += Vec2::SOUTH * POINT_MOVEMENT_SPEED;
	}
	if (g_input->IsKeyDown('D'))
	{
		m_launchPosition += Vec2::EAST * POINT_MOVEMENT_SPEED;
	}
	if (g_input->IsKeyDown('A'))
	{
		m_launchPosition += Vec2::WEST * POINT_MOVEMENT_SPEED;
	}

	// IJKL
	if (g_input->IsKeyDown('I'))
	{
		m_launchRayEnd += Vec2::NORTH * POINT_MOVEMENT_SPEED;
	}
	if (g_input->IsKeyDown('K'))
	{
		m_launchRayEnd += Vec2::SOUTH * POINT_MOVEMENT_SPEED;
	}
	if (g_input->IsKeyDown('L'))
	{
		m_launchRayEnd += Vec2::EAST * POINT_MOVEMENT_SPEED;
	}
	if (g_input->IsKeyDown('J'))
	{
		m_launchRayEnd += Vec2::WEST * POINT_MOVEMENT_SPEED;
	}

	if (g_input->IsKeyDown(KEYCODE_LMB))
	{
		m_launchPosition = g_input->GetCursorNormalizedPosition() * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);
	}
	if (g_input->IsKeyDown(KEYCODE_RMB))
	{
		m_launchRayEnd = g_input->GetCursorNormalizedPosition() * Vec2(WORLD_SIZE_X, WORLD_SIZE_Y);
	}
	if (g_input->WasKeyJustPressed(KEYCODE_SPACE))
	{
		LaunchBall();
	}
	if (g_input->IsKeyDown('N'))
	{
		LaunchBall();
	}
	if (g_input->WasKeyJustPressed('B'))
	{
		m_isFloorFallThrough = !m_isFloorFallThrough;
	}

	if (g_input->WasKeyJustPressed('T'))
	{
		m_gameClock->SetTimeScale(0.25f);
	}
	if (g_input->WasKeyJustReleased('T'))
	{
		m_gameClock->SetTimeScale(1.f);
	}
}

void VisualTestPachinkoMachine::UpdatePhysics(float deltaSeconds)
{
	UpdateBalls(deltaSeconds);
	PushBallsOutOfEachOther();
	PushBallsOutOfBumpers();
	PushBallsIntoWorldBounds();
}

void VisualTestPachinkoMachine::UpdateBalls(float deltaSeconds)
{
	static const Vec2 acceleration = Vec2::SOUTH * 30.f;
	for (int ballIndex = 0; ballIndex < (int)m_balls.size(); ballIndex++)
	{
		PachinkoBall*& ball = m_balls[ballIndex];
		ball->m_velocity += acceleration * deltaSeconds;
		ball->m_center += ball->m_velocity * deltaSeconds;
	}
}

void VisualTestPachinkoMachine::PushBallsOutOfEachOther()
{	
	for (int ballAIndex = 0; ballAIndex < (int)m_balls.size() - 1; ballAIndex++)
	{
		for (int ballBIndex = ballAIndex + 1; ballBIndex < (int)m_balls.size(); ballBIndex++)
		{
			PachinkoBall*& ballA = m_balls[ballAIndex];
			PachinkoBall*& ballB = m_balls[ballBIndex];

			BounceDiscsOffEachOther2D(ballA->m_center, ballA->m_radius, ballA->m_velocity, ballA->m_elasticity, ballB->m_center, ballB->m_radius, ballB->m_velocity, ballB->m_elasticity);
		}
	}
}

void VisualTestPachinkoMachine::PushBallsOutOfBumpers()
{
	for (int ballIndex = 0; ballIndex < (int)m_balls.size(); ballIndex++)
	{
		for (int bumperIndex = 0; bumperIndex < (int)BumperType::COUNT * NUM_BUMPERS_PER_TYPE; bumperIndex++)
		{
			PachinkoBumper* const& bumper = m_bumpers[bumperIndex];
			PachinkoBall*& ball = m_balls[ballIndex];

			if (!DoDiscsOverlap(bumper->m_boundingDiscCenter, bumper->m_boundingDiscRadius, ball->m_center, ball->m_radius))
			{
				continue;
			}

			bumper->BounceBall(*m_balls[ballIndex]);
		}
	}
}

void VisualTestPachinkoMachine::PushBallsIntoWorldBounds()
{
	for (int ballIndex = 0; ballIndex < (int)m_balls.size(); ballIndex++)
	{
		PachinkoBall* ball = m_balls[ballIndex];

		BounceDiscOffFixedAABB2(ball->m_center, ball->m_radius, ball->m_velocity, ball->m_elasticity, AABB2(Vec2::ZERO, Vec2::NORTH * WORLD_SIZE_Y), WALLS_ELASTICITY);
		BounceDiscOffFixedAABB2(ball->m_center, ball->m_radius, ball->m_velocity, ball->m_elasticity, AABB2(Vec2(WORLD_SIZE_X, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y)), WALLS_ELASTICITY);
		
		if (!m_isFloorFallThrough)
		{
			BounceDiscOffFixedAABB2(ball->m_center, ball->m_radius, ball->m_velocity, ball->m_elasticity, AABB2(Vec2::ZERO, Vec2::EAST * WORLD_SIZE_X), WALLS_ELASTICITY);
		}
		else
		{
			if (ball->m_center.y + ball->m_radius < 0.f)
			{
				ball->m_center.y = WORLD_SIZE_Y + ball->m_radius + WORLD_SIZE_Y * 0.1f;
			}
		}
	}
}

void VisualTestPachinkoMachine::Render() const
{
	g_renderer->BeginCamera(m_worldCamera);

	static std::vector<Vertex_PCU> verts;
	verts.reserve(10000);
	verts.clear();

	// Add verts for bumpers
	for (int bumperIndex = 0; bumperIndex < (int)BumperType::COUNT * NUM_BUMPERS_PER_TYPE; bumperIndex++)
	{
		m_bumpers[bumperIndex]->AddVerts(verts);
	}

	// Add verts for balls
	for (int ballIndex = 0; ballIndex < (int)m_balls.size(); ballIndex++)
	{
		m_balls[ballIndex]->AddVerts(verts);
	}

	// Add verts for launch
	AddVertsForArrow2D(verts, m_launchPosition, m_launchRayEnd, 2.f, 0.1f, Rgba8::YELLOW);
	AddVertsForRing2D(verts, m_launchPosition, BALL_MIN_RADIUS, 0.2f, Rgba8::STEEL_BLUE);
	AddVertsForRing2D(verts, m_launchPosition, BALL_MAX_RADIUS, 0.2f, Rgba8::STEEL_BLUE);

	// Draw
	g_renderer->SetBlendMode(BlendMode::ALPHA);
	g_renderer->SetDepthMode(DepthMode::DISABLED);
	g_renderer->SetModelConstants();
	g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_BACK);
	g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->BindShader(nullptr);
	g_renderer->BindTexture(nullptr);
	g_renderer->DrawVertexArray(verts);

	g_renderer->EndCamera(m_worldCamera);
}

void VisualTestPachinkoMachine::Randomize()
{
	AABB2 worldBox(Vec2::ZERO, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	worldBox.AddPadding(-WORLD_SIZE_X * 0.01f, -WORLD_SIZE_Y * 0.01f);

	m_balls.clear();

	m_launchPosition = g_RNG->RollRandomVec2InBox(worldBox);
	m_launchRayEnd = g_RNG->RollRandomVec2InBox(worldBox);
	m_isFloorFallThrough = false;

	for (int bumperTypeIndex = 0; bumperTypeIndex < (int)BumperType::COUNT; bumperTypeIndex++)
	{
		for (int bumperIndex = 0; bumperIndex < NUM_BUMPERS_PER_TYPE; bumperIndex++)
		{
			float bumperElasticity = g_RNG->RollRandomFloatInRange(0.1f, 0.9f);
			PachinkoBumper* bumper = new PachinkoBumper(bumperElasticity);

			switch (BumperType(bumperTypeIndex))
			{
				case BumperType::CAPSULE:
				{
					bumper->m_type = BumperType::CAPSULE;
					bumper->m_start = g_RNG->RollRandomVec2InBox(worldBox);
					Vec2 capsuleDirection = g_RNG->RollRandomVec2InRadius(Vec2::ZERO, 1.f);
					float capsuleLength = g_RNG->RollRandomFloatInRange(CAPSULE_MIN_LENGTH, CAPSULE_MAX_LENGTH);
					bumper->m_end = bumper->m_start + capsuleLength * capsuleDirection;
					bumper->m_radius = g_RNG->RollRandomFloatInRange(CAPSULE_MIN_RADIUS, CAPSULE_MAX_RADIUS);
					bumper->m_boundingDiscCenter = (bumper->m_start + bumper->m_end) * 0.5f;
					bumper->m_boundingDiscRadius = capsuleLength + 2.f * bumper->m_radius;
					break;
				}
				case BumperType::DISC:
				{
					bumper->m_type = BumperType::DISC;
					bumper->m_center = g_RNG->RollRandomVec2InBox(worldBox);
					bumper->m_radius = g_RNG->RollRandomFloatInRange(DISC_MIN_RADIUS, DISC_MAX_RADIUS);
					bumper->m_boundingDiscCenter = bumper->m_center;
					bumper->m_boundingDiscRadius = bumper->m_radius;
					break;
				}
				case BumperType::OBB2:
				{
					bumper->m_type = BumperType::OBB2;
					bumper->m_center = g_RNG->RollRandomVec2InBox(worldBox);
					bumper->m_iBasis = g_RNG->RollRandomVec2InRadius(Vec2::ZERO, 1.f).GetNormalized();
					bumper->m_halfDimensions = g_RNG->RollRandomVec2InRange(BOX_MIN_DIMENSION, BOX_MAX_DIMENSION, BOX_MIN_DIMENSION, BOX_MAX_DIMENSION);
					bumper->m_boundingDiscCenter = bumper->m_center;
					bumper->m_boundingDiscRadius = sqrtf(bumper->m_halfDimensions.x * bumper->m_halfDimensions.x + bumper->m_halfDimensions.y * bumper->m_halfDimensions.y);
					break;
				}
			}

			m_bumpers[bumperTypeIndex * NUM_BUMPERS_PER_TYPE + bumperIndex] = bumper;
		}
	}
}

void VisualTestPachinkoMachine::LaunchBall()
{
	PachinkoBall* ball = new PachinkoBall();
	ball->m_center = m_launchPosition;
	ball->m_radius = g_RNG->RollRandomFloatInRange(BALL_MIN_RADIUS, BALL_MAX_RADIUS);
	ball->m_velocity = (m_launchRayEnd - m_launchPosition);
	ball->m_elasticity = BALL_ELASTICITY;

	m_balls.push_back(ball);

	if (m_physicsTimer.IsStopped())
	{
		m_physicsTimer.Start();
	}
}
