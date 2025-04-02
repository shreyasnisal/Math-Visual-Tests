#include "Game/Game.hpp"

#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Game/VisualTest2DPrimitives.hpp"
#include "Game/VisualTestRaycastVsTiles.hpp"
#include "Game/VisualTestRaycastVsDiscs.hpp"
#include "Game/VisualTestRaycastVsLineSegments.hpp"
#include "Game/VisualTestRaycastVsAABB2.hpp"
#include "Game/VisualTest3DGeometry.hpp"
#include "Game/VisualTestSplines.hpp"
#include "Game/VisualTestPachinkoMachine.hpp"
#include "Game/VisualTestConvexScene.hpp"


#include "Engine/Math/RaycastUtils.hpp"

Game* Game::CreateNewGameOfMode(VisualTestMode mode)
{
	switch (mode)
	{
		case VisualTestMode::VISUAL_TEST_CONVEX_SCENE:					return new VisualTestConvexScene();
		case VisualTestMode::VISUAL_TEST_MODE_2D_PRIMITIVES:			return new VisualTest2DPrimitives();
		case VisualTestMode::VISUAL_TEST_MODE_RAYCAST_VS_TILES:			return new VisualTestRaycastVsTiles();
		case VisualTestMode::VISUAL_TEST_MODE_RAYCAST_VS_DISCS:			return new VisualTestRaycastVsDiscs();
		case VisualTestMode::VISUAL_TEST_MODE_RAYCAST_VS_LINESEGMENTS:	return new VisualTestRaycastVsLineSegments();
		case VisualTestMode::VISUAL_TEST_MODE_RAYCAST_VS_AABB2:			return new VisualTestRaycastVsAABB2();
		case VisualTestMode::VISUAL_TEST_MODE_3D_GEOMETRY:				return new VisualTest3DGeometry();
		case VisualTestMode::VISUAL_TEST_MODE_SPLINES:					return new VisualTestSplines();
		case VisualTestMode::VISUAL_TEST_MODE_PACHINKO_MACHINE:			return new VisualTestPachinkoMachine();
	}

	return nullptr;
}
