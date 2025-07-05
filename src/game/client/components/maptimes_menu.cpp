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
#include <game/client/render.h>
#include <game/localization.h>
#include <game/client/components/maptimes.h>

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

void CMapTimesMenu::RenderTitle(CUIRect TitleBar, const char *pTitle)
{
	// EXACT title rendering from scoreboard
	TitleBar.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 15.0f);
	
	const float TitleFontSize = 40.0f; // Slightly smaller title font
	char aFullTitle[128];
	const char *pCurrentMap = Client()->GetCurrentMap();
	if(pCurrentMap && pCurrentMap[0] != '\0')
		str_format(aFullTitle, sizeof(aFullTitle), "%s - %s", pTitle, pCurrentMap);
	else
		str_copy(aFullTitle, pTitle, sizeof(aFullTitle));
	
	TitleBar.VMargin(20.0f, &TitleBar);
	SLabelProperties Props;
	Props.m_MaxWidth = TitleBar.w;
	Props.m_EllipsisAtEnd = true;
	Ui()->DoLabel(&TitleBar, aFullTitle, TitleFontSize, TEXTALIGN_MC, Props);
}

void CMapTimesMenu::RenderMapTimes(CUIRect Scoreboard)
{
	// Get data from MapTimes component
	if(!GameClient()->m_MapTimes.HasValidData())
	{
		SLabelProperties Props;
		Props.m_MaxWidth = Scoreboard.w;
		Props.m_EllipsisAtEnd = true;
		Ui()->DoLabel(&Scoreboard, "Loading map times...", 24.0f, TEXTALIGN_MC, Props);
		return;
	}

	const int NumRecords = GameClient()->m_MapTimes.GetNumRecords();
	if(NumRecords == 0)
	{
		SLabelProperties Props;
		Props.m_MaxWidth = Scoreboard.w;
		Props.m_EllipsisAtEnd = true;
		Ui()->DoLabel(&Scoreboard, "No records available", 24.0f, TEXTALIGN_MC, Props);
		return;
	}

	// Configurable layout parameters (adjustable via console)
	const float LineHeight = (float)g_Config.m_ClMapTimesMenuLineHeight;
	const float FontSize = (float)g_Config.m_ClMapTimesMenuFontSize;
	const float RoundRadius = 5.0f;
	const float Spacing = (float)g_Config.m_ClMapTimesMenuSpacing;

	// Column layout EXACTLY like scoreboard
	const float RankOffset = Scoreboard.x + 40.0f;
	const float RankLength = TextRender()->TextWidth(FontSize, "99");
	const float TimeLength = TextRender()->TextWidth(FontSize, "00:00:00");
	const float TimeOffset = Scoreboard.x + Scoreboard.w - TimeLength - 20.0f;
	const float NameOffset = RankOffset + RankLength + 20.0f;
	const float NameLength = TimeOffset - NameOffset - 20.0f;

	// EXACT headline rendering from scoreboard
	const float HeadlineFontsize = 22.0f;
	CUIRect Headline;
	Scoreboard.HSplitTop(HeadlineFontsize * 2.0f, &Headline, &Scoreboard);
	const float HeadlineY = Headline.y + Headline.h / 2.0f - HeadlineFontsize / 2.0f;
	
	TextRender()->Text(RankOffset, HeadlineY, HeadlineFontsize, Localize("Rank"));
	TextRender()->Text(NameOffset, HeadlineY, HeadlineFontsize, Localize("Name"));
	const char *pTimeLabel = Localize("Time");
	TextRender()->Text(TimeOffset + TimeLength - TextRender()->TextWidth(HeadlineFontsize, pTimeLabel), HeadlineY, HeadlineFontsize, pTimeLabel);

	// Render exactly 10 entries (top 10 leaderboard)
	for(int i = 0; i < 10; i++)
	{
		const SMapTimeRecord *pRecord = GameClient()->m_MapTimes.GetRecord(i);
		
		CUIRect RowAndSpacing, Row;
		Scoreboard.HSplitTop(LineHeight + Spacing, &RowAndSpacing, &Scoreboard);
		RowAndSpacing.HSplitTop(LineHeight, &Row, nullptr);

		// No background for individual rows - cleaner look

		if(!pRecord)
		{
			// Empty slot - show placeholder
			char aRankText[16];
			str_format(aRankText, sizeof(aRankText), "%d.", i + 1);
			TextRender()->Text(RankOffset, Row.y + (Row.h - FontSize) / 2.0f, FontSize, aRankText);
			TextRender()->Text(NameOffset, Row.y + (Row.h - FontSize) / 2.0f, FontSize, "---");
			continue;
		}

		// Set colors for podium places (text only)
		ColorRGBA TextColor = TextRender()->DefaultTextColor();
		if(pRecord->m_Rank == 1)
			TextColor = ColorRGBA(1.0f, 0.84f, 0.0f, 1.0f); // Gold
		else if(pRecord->m_Rank == 2)
			TextColor = ColorRGBA(0.75f, 0.75f, 0.75f, 1.0f); // Silver
		else if(pRecord->m_Rank == 3)
			TextColor = ColorRGBA(0.8f, 0.52f, 0.25f, 1.0f); // Bronze
		
		TextRender()->TextColor(TextColor);

		// Simple rank rendering without medals - just numbers
		char aRankText[16];
		str_format(aRankText, sizeof(aRankText), "%d.", pRecord->m_Rank);
		TextRender()->Text(RankOffset, Row.y + (Row.h - FontSize) / 2.0f, FontSize, aRankText);

		// Name rendering with ellipsis
		{
			CTextCursor Cursor;
			TextRender()->SetCursor(&Cursor, NameOffset, Row.y + (Row.h - FontSize) / 2.0f, FontSize, TEXTFLAG_RENDER | TEXTFLAG_ELLIPSIS_AT_END);
			Cursor.m_LineWidth = NameLength;
			TextRender()->TextEx(&Cursor, pRecord->m_aPlayerName);
		}

		// Format and render time (right-aligned)
		char aFormattedTime[32];
		GameClient()->m_MapTimes.FormatTime(aFormattedTime, sizeof(aFormattedTime), pRecord->m_aTime);
		TextRender()->Text(TimeOffset + TimeLength - TextRender()->TextWidth(FontSize, aFormattedTime), Row.y + (Row.h - FontSize) / 2.0f, FontSize, aFormattedTime);

		// Reset text color to default
		TextRender()->TextColor(TextRender()->DefaultTextColor());
	}
}

void CMapTimesMenu::OnRender()
{
	if(!IsActive())
		return;

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	// EXACT coordinate system from scoreboard OnRender
	const float Height = 400.0f * 3.0f;
	const float Width = Height * Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, Width, Height);

	// Configurable size for Map Times (adjustable via console)
	const float ScoreboardWidth = (float)g_Config.m_ClMapTimesMenuWidth;
	const float ScoreboardHeight = (float)g_Config.m_ClMapTimesMenuHeight;
	const float TitleHeight = 60.0f;

	CUIRect Scoreboard = {(Width - ScoreboardWidth) / 2.0f, 150.0f, ScoreboardWidth, ScoreboardHeight + TitleHeight};
	
	// EXACT background from scoreboard
	Scoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 15.0f);

	// EXACT title split from scoreboard
	CUIRect TitleBar, MainBody;
	Scoreboard.HSplitTop(TitleHeight, &TitleBar, &MainBody);
	
	// Render title
	RenderTitle(TitleBar, "Map Times Leaderboard");
	
	// EXACT margins from scoreboard
	MainBody.VMargin(10.0f, &MainBody);
	MainBody.VMargin(10.0f, &MainBody);
	
	// Render map times using EXACT scoreboard logic
	RenderMapTimes(MainBody);
}

void CMapTimesMenu::OnRelease()
{
	m_Active = false;
}

bool CMapTimesMenu::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(!IsActive())
		return false;
	return true;
}

bool CMapTimesMenu::OnInput(const IInput::CEvent &Event)
{
	if(IsActive() && Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
	{
		SetActive(false);
		return true;
	}
	// Allow other inputs to pass through when leaderboard is open
	return false;
}

void CMapTimesMenu::Toggle()
{
	int64_t Now = time_get();
	if(Now - m_LastToggle < time_freq() / 10)
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
			GameClient()->m_MapTimes.OnMapLoad();
		}
	}
}
