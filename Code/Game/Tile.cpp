#include "Game/Tile.hpp"

#include "Game/GameCommon.hpp"

Tile::Tile(TileType type, IntVec2 tileCoords)
	: m_type(type)
	, m_tileCoords(tileCoords)
{
}

AABB2 const Tile::GetBounds() const
{
	return AABB2(Vec2(static_cast<float>(m_tileCoords.x), static_cast<float>(m_tileCoords.y)), Vec2(static_cast<float>(m_tileCoords.x + 1), static_cast<float>(m_tileCoords.y + 1)));
}

Rgba8 const Tile::GetColor() const
{
	Rgba8 color = Rgba8::MAGENTA;
	if (m_type == TileType::EMPTY)
	{
		color = Rgba8::TRANSPARENT_BLACK;
	}
	else if (m_type == TileType::SOLID)
	{
		color = Rgba8::WHITE;
	}

	return color;
}