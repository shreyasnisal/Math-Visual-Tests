#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Camera.hpp"

#include "Game/Game.hpp"

class App
{
public:
						App							();
						~App						();
	void				Startup						();
	void				Shutdown					();
	void				Run							();
	void				RunFrame					();

	bool				IsQuitting					() const		{ return m_isQuitting; }
	bool				HandleQuitRequested			();

public:
	Game*				m_game;
	VisualTestMode		m_currentMode = VisualTestMode::VISUAL_TEST_CONVEX_SCENE;
	Texture*			m_testTexture = nullptr;

private:
	void				BeginFrame					();
	void				Update						(float deltaSeconds);
	void				Render						() const;
	void				EndFrame					();

private:
	bool				m_isQuitting				= false;
	double				m_previousFrameTime			= 0;
};
