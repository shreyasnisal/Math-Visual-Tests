#pragma once

#include "Game/Game.hpp"

#include "Engine/Math/CubicBezierCurve2D.hpp"
#include "Engine/Math/CubicHermiteCurve2D.hpp"
#include "Engine/Math/Splines.hpp"

typedef float (*EasingFunction)(float t);

constexpr int NUM_EASING_FUNCTIONS = 17;

float CustomFunkyEasingFunction(float t);

const EasingFunction EASING_FUNCTIONS[] =
{
	SmoothStart2,
	SmoothStart3,
	SmoothStart4,
	SmoothStart5,
	SmoothStart6,
	SmoothStart7,
	SmoothStop2,
	SmoothStop3,
	SmoothStop4,
	SmoothStop5,
	SmoothStop6,
	SmoothStop7,
	SmoothStep3,
	SmoothStep5,
	Hesitate3,
	Hesitate5,
	CustomFunkyEasingFunction,
};

const std::string EASING_FUNCTION_NAMES[] =
{
	"Ease-In Quadratic (SmoothStart2)",
	"Ease-In Cubic (SmoothStart3)",
	"Ease-In Quartic (SmoothStart4)",
	"Ease-In Quintic (SmoothStart5)",
	"Ease-In Hexic (SmoothStart6)",
	"Ease-In Septic (SmoothStart7)",
	"Ease-Out Quadratic (SmoothStop2)",
	"Ease-Out Cubic (SmoothStop3)",
	"Ease-Out Quartic (SmoothStop4)",
	"Ease-Out Quintic (SmoothStop5)",
	"Ease-Out Hexic (SmoothStop6)",
	"Ease-Out Septic (SmoothStop7)",
	"Smooth Step (SmoothStep3)",
	"Smoother Step (SmoothStep5)",
	"Hesitate3",
	"Hesitate5",
	"Custom Easing Function [fabsf(SinDegrees(450.f * t))]"
};

class VisualTestSplines : public Game
{
public:
	~VisualTestSplines();
	VisualTestSplines();
	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;
	virtual void Randomize() override;

	void AddVertsForEasing(std::vector<Vertex_PCU>& verts, std::vector<Vertex_PCU>& textVerts) const;
	void AddVertsForBezier(std::vector<Vertex_PCU>& verts, std::vector<Vertex_PCU>& textVerts) const;
	void AddVertsForSpline(std::vector<Vertex_PCU>& verts, std::vector<Vertex_PCU>& textVerts) const;
	void GoToNextEasingFunction();
	void GoToPreviousEasingFunction();
	void IncrementSubdivisions();
	void DecrementSubdivisions();

public:
	AABB2 m_easingPane;
	AABB2 m_bezierPane;
	AABB2 m_splinePane;

	bool m_debugDraw = false;
	int m_easingFunctionIndex = 0;
	int m_numSubdivisions = 64;
	Clock* m_splineVisualTestClock = nullptr;

	CubicBezierCurve2D m_bezierCurve;
	CubicHermiteCurve2D m_hermiteCurve;
	CatmullRomSpline m_spline;
};
