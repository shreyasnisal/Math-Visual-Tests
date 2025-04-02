#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"

enum class TileType
{
	EMPTY,
	SOLID,
};

class Tile
{
public:
	~Tile() = default;
	Tile() = default;
	explicit Tile(TileType type, IntVec2 tileCoords);

	bool IsSolid() const { return m_type == TileType::SOLID; }
	TileType GetType() const { return m_type; }
	AABB2 const GetBounds() const;
	Rgba8 const GetColor() const;

public:
	TileType m_type;
	IntVec2 m_tileCoords;
};