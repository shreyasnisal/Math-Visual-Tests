#include "Game/App.hpp"

#include "Game/GameCommon.hpp"

#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"


App* g_app = nullptr;
RandomNumberGenerator* g_RNG = nullptr;
Renderer* g_renderer = nullptr;
Window* g_window = nullptr;

App::App()
{

}

App::~App()
{
	delete g_renderer;
	g_renderer = nullptr;

	delete g_input;
	g_input = nullptr;

	delete g_RNG;
	g_RNG = nullptr;

	delete m_game;
	m_game = nullptr;
}

void App::Startup()
{
	EventSystemConfig eventSystemConfig;
	g_eventSystem = new EventSystem(eventSystemConfig);

	InputConfig inputConfig;
	g_input = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_input;
	//windowConfig.m_isFullScreen = true;
	windowConfig.m_windowTitle = "(snisal) Math Visual Tests";
	windowConfig.m_clientAspect = 2.f;
	g_window = new Window(windowConfig);

	RenderConfig renderConfig;
	renderConfig.m_window = g_window;
	g_renderer = new Renderer(renderConfig);

	DevConsoleConfig devConsoleConfig;
	Camera devConsoleCamera = Camera();
	devConsoleCamera.SetOrthoView(Vec2::ZERO, Vec2(2.f, 1.f));
	devConsoleConfig.m_camera = devConsoleCamera;
	devConsoleConfig.m_renderer = g_renderer;
	devConsoleConfig.m_consoleFontFilePathWithNoExtension = "Data/Images/SquirrelFixedFont";
	g_console = new DevConsole(devConsoleConfig);

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_bitmapFontFilePathWithNoExtension = "Data/Images/SquirrelFixedFont";
	debugRenderConfig.m_renderer = g_renderer;

	g_eventSystem->Startup();
	g_console->Startup();
	g_input->Startup();
	g_window->Startup();
	g_renderer->Startup();
	DebugRenderSystemStartup(debugRenderConfig);

	m_testTexture = g_renderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
	g_squirrelFont = g_renderer->CreateOrGetBitmapFont("Data/Images/SquirrelFixedFont");
	//m_testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/TestUV.png");

	m_game = Game::CreateNewGameOfMode(m_currentMode);
}

void App::Run()
{
	while (!IsQuitting())
	{
		RunFrame();
	}
}

void App::RunFrame()
{
	double timeNow = GetCurrentTimeSeconds();
	float deltaSeconds = GetClamped(static_cast<float>(timeNow - m_previousFrameTime), 0, 0.1f);
	m_previousFrameTime = timeNow;

	BeginFrame();
	Update(deltaSeconds);
	Render();
	EndFrame();

}

bool App::HandleQuitRequested()
{
	m_isQuitting = true;

	return true;
}

void App::BeginFrame()
{	
	Clock::TickSystemClock();

	g_eventSystem->Startup();
	g_console->Startup();
	g_input->BeginFrame();
	g_window->BeginFrame();
	g_renderer->BeginFrame();
	DebugRenderBeginFrame();

	DebugAddScreenText(Stringf("FPS: %.0f", 1.f / Clock::GetSystemClock().GetDeltaSeconds()), Vec2(SCREEN_SIZE_X, 16.f), 16.f, Vec2(1.2f, 1.f), 0.f);
}

void App::Update(float deltaSeconds)
{
	m_game->Update(deltaSeconds);

	if (g_input->WasKeyJustPressed(KEYCODE_ESC))
	{
		HandleQuitRequested();
	}
	if (g_input->WasKeyJustPressed(KEYCODE_F8))
	{
		m_game->Randomize();
	}
	if (g_input->WasKeyJustPressed(KEYCODE_F6))
	{
		m_currentMode = VisualTestMode(((int)m_currentMode - 1));
		if (m_currentMode == VisualTestMode::VISUAL_TEST_MODE_INVALID)
		{
			m_currentMode = VisualTestMode((int)VisualTestMode::VISUAL_TEST_MODE_NUM - 1);
		}
		delete m_game;
		m_game = Game::CreateNewGameOfMode(m_currentMode);
	}
	if (g_input->WasKeyJustPressed(KEYCODE_F7))
	{
		m_currentMode = VisualTestMode((int)m_currentMode + 1);
		if (m_currentMode == VisualTestMode::VISUAL_TEST_MODE_NUM)
		{
			m_currentMode = VisualTestMode(0);
		}
		delete m_game;
		m_game = Game::CreateNewGameOfMode(m_currentMode);
	}
}

void App::Render() const
{
	g_renderer->ClearScreen(Rgba8(0, 0, 0, 255));
	m_game->Render();
	DebugRenderWorld(m_game->m_worldCamera);
	DebugRenderScreen(m_game->m_screenCamera);
}

void App::EndFrame()
{
	DebugRenderEndFrame();
	g_renderer->EndFrame();
	g_window->EndFrame();
	g_input->EndFrame();
	g_console->EndFrame();
	g_eventSystem->EndFrame();
}

void App::Shutdown()
{
	DebugRenderSystemShutdown();
	g_renderer->Shutdown();
	g_window->Shutdown();
	g_input->Shutdown();
	g_console->Shutdown();
	g_eventSystem->Shutdown();
}

