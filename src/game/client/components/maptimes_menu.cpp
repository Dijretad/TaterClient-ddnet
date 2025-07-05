/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "maptimes_menu.h"

#include <base/log.h>
#include <base/math.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>

CMapTimesMenu::CMapTimesMenu()
{
	m_Active = false;
	m_WasActive = false;
	m_LastToggle = 0;
}

void CMapTimesMenu::OnInit()
{
	// Nothing to initialize for now
}

void CMapTimesMenu::OnRender()
{
	if(!IsActive())
		return;

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	// Setup screen coordinates
	Graphics()->MapScreen(0, 0, Graphics()->ScreenWidth(), Graphics()->ScreenHeight());

	// Dark background overlay
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.5f);
	IGraphics::CQuadItem BackgroundQuad(0, 0, Graphics()->ScreenWidth(), Graphics()->ScreenHeight());
	Graphics()->QuadsBegin();
	Graphics()->QuadsDraw(&BackgroundQuad, 1);
	Graphics()->QuadsEnd();

	// Center the map times widget
	float CenterX = Graphics()->ScreenWidth() / 2.0f - 200.0f; // Offset for centering
	float CenterY = Graphics()->ScreenHeight() / 2.0f - 150.0f;

	// Render the map times widget in the center
	GameClient()->m_MapTimes.RenderMapTimesFullscreen(CenterX, CenterY);

	// Instructions at the bottom
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.7f);
	
	const char *pInstructions = "Press ESC or assigned key to close";
	float InstructionsWidth = TextRender()->TextWidth(16.0f, pInstructions, -1, -1.0f);
	float InstructionsX = Graphics()->ScreenWidth() / 2.0f - InstructionsWidth / 2.0f;
	float InstructionsY = Graphics()->ScreenHeight() - 50.0f;
	
	TextRender()->Text(InstructionsX, InstructionsY, 16.0f, pInstructions);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CMapTimesMenu::OnRelease()
{
	m_Active = false;
}

bool CMapTimesMenu::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(!IsActive())
		return false;
	return true; // Consume cursor movement when active
}

bool CMapTimesMenu::OnInput(const IInput::CEvent &Event)
{
	// Check for map times key press when menu is closed
	if(!IsActive() && Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == g_Config.m_ClMapTimesKey)
	{
		SetActive(true);
		return true;
	}
	
	// Handle input when menu is active
	if(!IsActive())
		return false;

	if(Event.m_Flags & IInput::FLAG_PRESS)
	{
		// Close with ESC
		if(Event.m_Key == KEY_ESCAPE)
		{
			SetActive(false);
			return true;
		}
		
		// Close with assigned map times key
		if(Event.m_Key == g_Config.m_ClMapTimesKey)
		{
			SetActive(false);
			return true;
		}
	}

	return true; // Consume all input when active
}

void CMapTimesMenu::Toggle()
{
	int64_t Now = time_get();
	if(Now - m_LastToggle < time_freq() / 10) // 100ms cooldown
		return;
	
	m_LastToggle = Now;
	SetActive(!m_Active);
}

void CMapTimesMenu::SetActive(bool Active)
{
	if(m_Active != Active)
	{
		m_WasActive = m_Active;
		m_Active = Active;
		
		if(m_Active)
		{
			// When opening, ensure we have fresh data
			GameClient()->m_MapTimes.OnMapLoad();
		}
	}
}
