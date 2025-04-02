#include "Game/VisualTestSplines.hpp"

#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

constexpr int NUM_SUBDIVISIONS_FOR_SMOOTH_CURVES = 64;
constexpr int NUM_POINTS_IN_SPLINE = 5;

VisualTestSplines::~VisualTestSplines()
{
	Clock::GetSystemClock().RemoveChild(m_splineVisualTestClock);
	delete m_splineVisualTestClock;
	m_splineVisualTestClock = nullptr;
}

VisualTestSplines::VisualTestSplines()
{
	g_input->SetCursorMode(false, false);

	m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
	m_worldCamera.SetOrthoView(Vec2::ZERO, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));

	AABB2 worldBox = AABB2(Vec2::ZERO, Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	worldBox.Translate(Vec2::SOUTH * WORLD_SIZE_Y * 0.025f);
	worldBox.AddPadding(-WORLD_SIZE_X * 0.02f, -WORLD_SIZE_Y * 0.04f);
	m_easingPane = worldBox.GetBoxAtUVs(Vec2(0.f, 0.51f), Vec2(0.49f, 1.f));
	m_bezierPane = worldBox.GetBoxAtUVs(Vec2(0.51f, 0.51f), Vec2(1.f, 1.f));
	m_splinePane = worldBox.GetBoxAtUVs(Vec2(0.f, 0.f), Vec2(1.f, 0.49f));

	m_splineVisualTestClock = new Clock();
	m_splineVisualTestClock->SetTimeScale(0.5f);

	Randomize();
}

void VisualTestSplines::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_input->WasKeyJustPressed('W'))
	{
		GoToPreviousEasingFunction();
	}
	if (g_input->WasKeyJustPressed('E'))
	{
		GoToNextEasingFunction();
	}
	if (g_input->WasKeyJustPressed('N'))
	{
		DecrementSubdivisions();
	}
	if (g_input->WasKeyJustPressed('M'))
	{
		IncrementSubdivisions();
	}
	if (g_input->WasKeyJustPressed(KEYCODE_F1))
	{
		m_debugDraw = !m_debugDraw;
	}

	if (g_input->WasKeyJustPressed('T'))
	{
		m_splineVisualTestClock->SetTimeScale(0.05f);
	}
	if (g_input->WasKeyJustReleased('T'))
	{
		m_splineVisualTestClock->SetTimeScale(0.5f);
	}

	DebugAddMessage(Stringf("F8 to Randomize; W/E : prev/next Easing Function; N/M : Subdivisions (%d); hold T = slow", m_numSubdivisions), 0.f, Rgba8::CYAN, Rgba8::CYAN);
	DebugAddMessage("Mode [F6/F7 for Prev/Next]: Splines (2D)", 0.f, Rgba8::YELLOW, Rgba8::YELLOW);
}

void VisualTestSplines::Render() const
{
	g_renderer->BeginCamera(m_worldCamera);

	std::vector<Vertex_PCU> splineVisualTestVerts;
	std::vector<Vertex_PCU> splineVisualTestTextVerts;

	if (m_debugDraw)
	{
		AddVertsForAABB2(splineVisualTestVerts, m_easingPane, Rgba8::MAROON);
		AddVertsForAABB2(splineVisualTestVerts, m_bezierPane, Rgba8::MAROON);
		AddVertsForAABB2(splineVisualTestVerts, m_splinePane, Rgba8::MAROON);
	}

	AddVertsForEasing(splineVisualTestVerts, splineVisualTestTextVerts);
	AddVertsForBezier(splineVisualTestVerts, splineVisualTestTextVerts);
	AddVertsForSpline(splineVisualTestVerts, splineVisualTestTextVerts);

	g_renderer->BindTexture(nullptr);
	g_renderer->BindShader(nullptr);
	g_renderer->SetModelConstants();
	g_renderer->SetDepthMode(DepthMode::DISABLED);
	g_renderer->SetBlendMode(BlendMode::ALPHA);
	g_renderer->SetRasterizerCullMode(RasterizerCullMode::CULL_NONE);
	g_renderer->SetRasterizerFillMode(RasterizerFillMode::SOLID);
	g_renderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_renderer->DrawVertexArray(splineVisualTestVerts);

	g_renderer->BindTexture(g_squirrelFont->GetTexture());
	g_renderer->DrawVertexArray(splineVisualTestTextVerts);

	g_renderer->EndCamera(m_worldCamera);
}

void VisualTestSplines::Randomize()
{
	m_easingFunctionIndex = g_RNG->RollRandomIntInRange(0, NUM_EASING_FUNCTIONS - 1);

	Vec2 curveStart = g_RNG->RollRandomVec2InBox(m_bezierPane);
	Vec2 curveGuide1 = g_RNG->RollRandomVec2InBox(m_bezierPane);
	Vec2 curveGuide2 = g_RNG->RollRandomVec2InBox(m_bezierPane);
	//Vec2 curveStartVelocity = g_RNG->RollRandomVec2InRadius(Vec2::ZERO, 35.f);
	Vec2 curveEnd = g_RNG->RollRandomVec2InBox(m_bezierPane);
	//Vec2 curveEndVelocity = g_RNG->RollRandomVec2InRadius(Vec2::ZERO, 35.f);
	//m_hermiteCurve = CubicHermiteCurve2D(curveStart, curveStartVelocity, curveEnd, curveEndVelocity);
	//m_bezierCurve = CubicBezierCurve2D(m_hermiteCurve);
	m_bezierCurve = CubicBezierCurve2D(curveStart, curveGuide1, curveGuide2, curveEnd);
	m_hermiteCurve = CubicHermiteCurve2D(m_bezierCurve);


	// Generate Catmull Rom Spline
	std::vector<Vec2> positions;
	float pointU = 0.1f;
	float uStep = 1.f / NUM_POINTS_IN_SPLINE;
	for (int pointIndex = 0; pointIndex < NUM_POINTS_IN_SPLINE; pointIndex++)
	{
		float pointV = g_RNG->RollRandomFloatInRange(0.1f, 0.9f);
		Vec2 point = m_splinePane.GetPointAtUV(Vec2(pointU, pointV));
		positions.push_back(point);
		pointU += uStep;
	}

	m_spline = CatmullRomSpline(positions);
}

void VisualTestSplines::AddVertsForEasing(std::vector<Vertex_PCU>& verts, std::vector<Vertex_PCU>& textVerts) const
{
	AABB2 easingBox(m_easingPane);
	easingBox.ReduceToAspect(1.f);
	easingBox.AddPadding(-2.f, -2.f);
	easingBox.Translate(Vec2::NORTH * 2.f);
	AddVertsForAABB2(verts, easingBox, Rgba8::NAVY);
	AABB2 easingNameBox(m_easingPane.m_mins, Vec2(m_easingPane.m_maxs.x, m_easingPane.m_mins.y + 4.f));

	float t = m_splineVisualTestClock->GetTotalSeconds();
	while (t > 1.f)
	{
		t -= 1.f;
	}

	EasingFunction easingFunction = EASING_FUNCTIONS[m_easingFunctionIndex];
	std::string easingFunctionName = EASING_FUNCTION_NAMES[m_easingFunctionIndex];

	float tStepPerSubdivision = 1.f / (float)NUM_SUBDIVISIONS_FOR_SMOOTH_CURVES;
	float pointU = 0.f;
	for (int segmentIndex = 0; segmentIndex < NUM_SUBDIVISIONS_FOR_SMOOTH_CURVES; segmentIndex++)
	{
		Vec2 startUVs = Vec2(pointU, easingFunction(pointU));
		pointU += tStepPerSubdivision;
		Vec2 endUVs = Vec2(pointU, easingFunction(pointU));
		Vec2 startPoint = easingBox.GetPointAtUV(startUVs);
		Vec2 endPoint = easingBox.GetPointAtUV(endUVs);
		AddVertsForLineSegment2D(verts, startPoint, endPoint, 0.2f, Rgba8::GRAY);
	}

	tStepPerSubdivision = 1.f / (float)m_numSubdivisions;
	pointU = 0.f;
	for (int segmentIndex = 0; segmentIndex < m_numSubdivisions; segmentIndex++)
	{
		Vec2 startUVs = Vec2(pointU, easingFunction(pointU));
		pointU += tStepPerSubdivision;
		Vec2 endUVs = Vec2(pointU, easingFunction(pointU));
		Vec2 startPoint = easingBox.GetPointAtUV(startUVs);
		Vec2 endPoint = easingBox.GetPointAtUV(endUVs);
		AddVertsForLineSegment2D(verts, startPoint, endPoint, 0.2f, Rgba8::GREEN);
	}

	float tOut = easingFunction(t);
	Vec2 pointPosition = easingBox.GetPointAtUV(Vec2(t, tOut));
	AddVertsForDisc2D(verts, pointPosition, 0.5f, Rgba8::WHITE);
	g_squirrelFont->AddVertsForTextInBox2D(textVerts, easingNameBox, 2.f, easingFunctionName, Rgba8::GREEN, 0.7f, Vec2(0.5f, 0.5f));
}

void VisualTestSplines::AddVertsForBezier(std::vector<Vertex_PCU>& verts, std::vector<Vertex_PCU>& textVerts) const
{
	UNUSED(textVerts);

	m_bezierCurve.AddVertsForDebugDraw(verts, Rgba8::GRAY, Rgba8::ORANGE, false, Rgba8::RED, NUM_SUBDIVISIONS_FOR_SMOOTH_CURVES, 0.2f, 0.5f);
	m_bezierCurve.AddVertsForDebugDraw(verts, Rgba8::GREEN, Rgba8::ORANGE, false, Rgba8::RED, m_numSubdivisions);

	float t = m_splineVisualTestClock->GetTotalSeconds();
	while (t > 1.f)
	{
		t -= 1.f;
	}

	float curveApproxLength = m_bezierCurve.GetApproximateLength(m_numSubdivisions);
	float greenPointVelocity = curveApproxLength / 1.f;
	float greenPointDistanceAlongCurve = greenPointVelocity * t;
	Vec2 greenPointPosition = m_bezierCurve.EvaluateAtApproximateDistance(greenPointDistanceAlongCurve, m_numSubdivisions);
	AddVertsForDisc2D(verts, greenPointPosition, 0.5f, Rgba8::GREEN);

	Vec2 curvePointPosition = m_bezierCurve.EvaluateAtParametric(t);
	AddVertsForDisc2D(verts, curvePointPosition, 0.5f, Rgba8::WHITE);
}

void VisualTestSplines::AddVertsForSpline(std::vector<Vertex_PCU>& verts, std::vector<Vertex_PCU>& textVerts) const
{
	UNUSED(textVerts);
	float t = m_splineVisualTestClock->GetTotalSeconds();
	while (t > (float)m_spline.m_positions.size() - 1)
	{
		t -= (float)m_spline.m_positions.size() - 1;
	}

	m_spline.AddVertsForDebugDraw(verts, Rgba8::GRAY, Rgba8::ORANGE, false, Rgba8::RED, NUM_SUBDIVISIONS_FOR_SMOOTH_CURVES, 0.1f, 0.5f);
	m_spline.AddVertsForDebugDraw(verts, Rgba8::GREEN, Rgba8::ORANGE, true, Rgba8::RED, m_numSubdivisions, 0.2f, 0.5f);
	
	float curveApproxLength = m_spline.GetApproximateLength(m_numSubdivisions);
	constexpr int NUM_CURVES_IN_SPLINE = NUM_POINTS_IN_SPLINE - 1;
	float greenPointVelocity = curveApproxLength / (float)NUM_CURVES_IN_SPLINE;
	float greenPointDistanceAlongCurve = greenPointVelocity * t;
	Vec2 greenPointPosition = m_spline.EvaluateAtApproximateDistance(greenPointDistanceAlongCurve, m_numSubdivisions);
	AddVertsForDisc2D(verts, greenPointPosition, 0.5f, Rgba8::GREEN);
	
	Vec2 curvePointPosition = m_spline.EvaluateAtParametric(t);
	AddVertsForDisc2D(verts, curvePointPosition, 0.5f, Rgba8::WHITE);
}

void VisualTestSplines::GoToNextEasingFunction()
{
	m_easingFunctionIndex = (m_easingFunctionIndex + 1) % NUM_EASING_FUNCTIONS;
}

void VisualTestSplines::GoToPreviousEasingFunction()
{
	m_easingFunctionIndex--;
	if (m_easingFunctionIndex < 0)
	{
		m_easingFunctionIndex = NUM_EASING_FUNCTIONS - 1;
	}
}

void VisualTestSplines::IncrementSubdivisions()
{
	if (m_numSubdivisions < 4096)
	{
		m_numSubdivisions = m_numSubdivisions << 1;
	}
}

void VisualTestSplines::DecrementSubdivisions()
{
	if (m_numSubdivisions > 2)
	{
		m_numSubdivisions = m_numSubdivisions >> 1;
	}
}

float CustomFunkyEasingFunction(float t)
{
	return fabsf(SinDegrees(450.f * t));
}
