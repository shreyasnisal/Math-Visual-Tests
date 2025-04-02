#include "Game/GameCommon.hpp"

BitmapFont* g_squirrelFont = nullptr;

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color)
{
	constexpr int RING_TRAPEZOIDS = 64;
	constexpr int VERTEXES_PER_TRAPEZOID = 6;
	constexpr float THETA_INCREMENT_DEGREES = 360.f / (float)(RING_TRAPEZOIDS);
	float innerRadius = radius - thickness * 0.5f;
	float outerRadius = radius + thickness * 0.5f;
	Vertex_PCU ringVerts[RING_TRAPEZOIDS * 6] = {};

	for (int trapIndex = 0; trapIndex < RING_TRAPEZOIDS; trapIndex++)
	{
		float startThetaDegrees =	(float)(trapIndex)		*	THETA_INCREMENT_DEGREES;
		float endThetaDegrees =		(float)(trapIndex + 1)	*	THETA_INCREMENT_DEGREES;

		Vec2 innerStartVertexPos	= Vec2::MakeFromPolarDegrees	(startThetaDegrees,		innerRadius);
		Vec2 outerStartVertexPos	= Vec2::MakeFromPolarDegrees	(startThetaDegrees,		outerRadius);
		Vec2 innerEndVertexPos		= Vec2::MakeFromPolarDegrees	(endThetaDegrees,		innerRadius);
		Vec2 outerEndVertexPos		= Vec2::MakeFromPolarDegrees	(endThetaDegrees,		outerRadius);

		Vertex_PCU verts[VERTEXES_PER_TRAPEZOID] = {
			Vertex_PCU		(Vec3(innerStartVertexPos.x,	innerStartVertexPos.y,	0.f),	color, Vec2(0.f, 0.f)),
			Vertex_PCU		(Vec3(outerStartVertexPos.x,	outerStartVertexPos.y,	0.f),	color, Vec2(0.f, 0.f)),
			Vertex_PCU		(Vec3(innerEndVertexPos.x,		innerEndVertexPos.y,	0.f),	color, Vec2(0.f, 0.f)),
			/**/
			Vertex_PCU		(Vec3(outerStartVertexPos.x,	outerStartVertexPos.y,	0.f),	color, Vec2(0.f, 0.f)),
			Vertex_PCU		(Vec3(innerEndVertexPos.x,		innerEndVertexPos.y, 0.f),		color, Vec2(0.f, 0.f)),
			Vertex_PCU		(Vec3(outerEndVertexPos.x, outerEndVertexPos.y, 0.f),			color, Vec2(0.f, 0.f)),
		};

		TransformVertexArrayXY3D(VERTEXES_PER_TRAPEZOID, verts, 1.f, 0.f, center);
		g_renderer->DrawVertexArray(VERTEXES_PER_TRAPEZOID, verts);
	}
}

void DebugDrawDisc(Vec2 const& center, float radius, Rgba8 const& color)
{
	constexpr int NUM_VERTEXES = 69;
	Vertex_PCU discVerts[NUM_VERTEXES];
	Vec3 previousVertexPosition = Vec3(0.f, 0.f, 0.f);
	for (int vertexIndex = 0; vertexIndex < NUM_VERTEXES; vertexIndex++)
	{
		if (vertexIndex % 3 == 0)
		{
			discVerts[vertexIndex] = Vertex_PCU(Vec3(0.f, 0.f, 0.f), color, Vec2(0.f, 0.f));
		}
		else if (vertexIndex % 3 == 1)
		{
			if (previousVertexPosition.x == 0 && previousVertexPosition.y == 0)
			{
				Vec2 vertexPosition = Vec2::MakeFromPolarDegrees(0.f, radius);
				discVerts[vertexIndex] = Vertex_PCU(Vec3(vertexPosition.x, vertexPosition.y, 0.f), color, Vec2(0.f, 0.f));
			}
			else
			{
				discVerts[vertexIndex] = Vertex_PCU(previousVertexPosition, color, Vec2(0.f, 0.f));
			}
		}
		else
		{
			Vec2 vertexPosition = Vec2::MakeFromPolarDegrees((1 + vertexIndex) * 360.f / NUM_VERTEXES, radius);
			discVerts[vertexIndex] = Vertex_PCU(Vec3(vertexPosition.x, vertexPosition.y, 0.f), color, Vec2(0.f, 0.f));
			previousVertexPosition = Vec3(vertexPosition.x, vertexPosition.y, 0.f);
		}
	}
	TransformVertexArrayXY3D(NUM_VERTEXES, discVerts, 1.f, 0.f, center);
	g_renderer->DrawVertexArray(NUM_VERTEXES, discVerts);
}

void DebugDrawGlow(Vec2 const& center, float radius, Rgba8 const& centerColor, Rgba8 const& edgeColor)
{
	constexpr int NUM_VERTEXES = 69;
	Vertex_PCU discVerts[NUM_VERTEXES];
	Vec3 previousVertexPosition = Vec3(0.f, 0.f, 0.f);
	for (int vertexIndex = 0; vertexIndex < NUM_VERTEXES; vertexIndex++)
	{
		if (vertexIndex % 3 == 0)
		{
			discVerts[vertexIndex] = Vertex_PCU(Vec3(0.f, 0.f, 0.f), centerColor, Vec2(0.f, 0.f));
		}
		else if (vertexIndex % 3 == 1)
		{
			if (previousVertexPosition.x == 0 && previousVertexPosition.y == 0)
			{
				Vec2 vertexPosition = Vec2::MakeFromPolarDegrees(0.f, radius);
				discVerts[vertexIndex] = Vertex_PCU(Vec3(vertexPosition.x, vertexPosition.y, 0.f), edgeColor, Vec2(0.f, 0.f));
			}
			else
			{
				discVerts[vertexIndex] = Vertex_PCU(previousVertexPosition, edgeColor, Vec2(0.f, 0.f));
			}
		}
		else
		{
			Vec2 vertexPosition = Vec2::MakeFromPolarDegrees((1 + vertexIndex) * 360.f / NUM_VERTEXES, radius);
			discVerts[vertexIndex] = Vertex_PCU(Vec3(vertexPosition.x, vertexPosition.y, 0.f), edgeColor, Vec2(0.f, 0.f));
			previousVertexPosition = Vec3(vertexPosition.x, vertexPosition.y, 0.f);
		}
	}
	TransformVertexArrayXY3D(NUM_VERTEXES, discVerts, 1.f, 0.f, center);
	g_renderer->DrawVertexArray(NUM_VERTEXES, discVerts);
}

void DebugDrawLine(Vec2 const& pointA, Vec2 const& pointB, float thickness, Rgba8 const& color)
{
	constexpr int NUM_VERTEXES = 6;
	float lineDirectionDegrees = Atan2Degrees(pointB.y - pointA.y, pointB.x - pointA.x);

	for (int vertexIndex = 0; vertexIndex < NUM_VERTEXES; vertexIndex++)
	{
		Vec3 vertex1Position = Vec3		(0.f,								thickness * 0.5f, 0.f);
		Vec3 vertex2Position = Vec3		(0.f,								-thickness * 0.5f, 0.f);
		Vec3 vertex3Position = Vec3		(GetDistance2D(pointA, pointB),		thickness * 0.5f, 0.f);
		Vec3 vertex4Position = Vec3		(GetDistance2D(pointA, pointB),		-thickness * 0.5f, 0.f);

		Vertex_PCU verts[NUM_VERTEXES] = {
			Vertex_PCU(Vec3(vertex1Position.x, vertex1Position.y, 0.f), color, Vec2(0.f, 0.f)),
			Vertex_PCU(Vec3(vertex2Position.x, vertex2Position.y, 0.f), color, Vec2(0.f, 0.f)),
			Vertex_PCU(Vec3(vertex3Position.x, vertex3Position.y, 0.f), color, Vec2(0.f, 0.f)),
			/**/
			Vertex_PCU(Vec3(vertex2Position.x, vertex2Position.y, 0.f), color, Vec2(0.f, 0.f)),
			Vertex_PCU(Vec3(vertex3Position.x, vertex3Position.y, 0.f), color, Vec2(0.f, 0.f)),
			Vertex_PCU(Vec3(vertex4Position.x, vertex4Position.y, 0.f), color, Vec2(0.f, 0.f)),
		};

		TransformVertexArrayXY3D(NUM_VERTEXES, verts, 1.f, lineDirectionDegrees, pointA);
		g_renderer->DrawVertexArray(NUM_VERTEXES, verts);
	}
}

void DrawBoxOutline(AABB2 const& boundedBox, float thickness, Rgba8 const& color)
{
	Vec2 const& bottomLeft = boundedBox.m_mins;
	Vec2 const& topRight = boundedBox.m_maxs;

	DebugDrawLine(bottomLeft, Vec2(topRight.x, bottomLeft.y), thickness, color);
	DebugDrawLine(Vec2(topRight.x, bottomLeft.y), topRight, thickness, color);
	DebugDrawLine(topRight, Vec2(bottomLeft.x, topRight.y), thickness, color);
	DebugDrawLine(Vec2(bottomLeft.x, topRight.y), bottomLeft, thickness, color);
}