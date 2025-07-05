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
	// Exact copy from scoreboard RenderTitle but simplified
	TitleBar.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 15.0f);
	
	const float TitleFontSize = 40.0f;
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
	// EXACT copy from scoreboard RenderScoreboard, but using Map Times data instead of player data
	
	// Get map times data
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

	const bool TimeScore = true; // Always true for map times
	const int NumPlayers = NumRecords; // Use number of records instead of players
	const bool LowScoreboardWidth = Scoreboard.w < 700.0f;

	// EXACT calculation from scoreboard
	float LineHeight;
	float TeeSizeMod;
	float Spacing;
	float RoundRadius;
	float FontSize;
	if(NumPlayers <= 8)
	{
		LineHeight = 60.0f;
		TeeSizeMod = 1.0f;
		Spacing = 16.0f;
		RoundRadius = 10.0f;
		FontSize = 24.0f;
	}
	else if(NumPlayers <= 12)
	{
		LineHeight = 50.0f;
		TeeSizeMod = 0.9f;
		Spacing = 5.0f;
		RoundRadius = 10.0f;
		FontSize = 24.0f;
	}
	else if(NumPlayers <= 16)
	{
		LineHeight = 40.0f;
		TeeSizeMod = 0.8f;
		Spacing = 0.0f;
		RoundRadius = 5.0f;
		FontSize = 24.0f;
	}
	else if(NumPlayers <= 24)
	{
		LineHeight = 27.0f;
		TeeSizeMod = 0.6f;
		Spacing = 0.0f;
		RoundRadius = 5.0f;
		FontSize = 20.0f;
	}
	else if(NumPlayers <= 32)
	{
		LineHeight = 20.0f;
		TeeSizeMod = 0.4f;
		Spacing = 0.0f;
		RoundRadius = 5.0f;
		FontSize = 16.0f;
	}
	else if(LowScoreboardWidth)
	{
		LineHeight = 15.0f;
		TeeSizeMod = 0.25f;
		Spacing = 0.0f;
		RoundRadius = 2.0f;
		FontSize = 14.0f;
	}
	else
	{
		LineHeight = 10.0f;
		TeeSizeMod = 0.2f;
		Spacing = 0.0f;
		RoundRadius = 2.0f;
		FontSize = 10.0f;
	}

	// EXACT layout from scoreboard but adapted for rank/name/time
	const float ScoreOffset = Scoreboard.x + 40.0f;
	const float ScoreLength = TextRender()->TextWidth(FontSize, TimeScore ? "00:00:00" : "99999");
	const float TeeOffset = ScoreOffset + ScoreLength + 20.0f;
	const float TeeLength = 60.0f * TeeSizeMod;
	const float NameOffset = TeeOffset + TeeLength;
	const float NameLength = (LowScoreboardWidth ? 180.0f : 300.0f) - TeeLength;
	const float CountryLength = (LineHeight - Spacing - TeeSizeMod * 5.0f) * 2.0f;
	const float PingLength = 55.0f;
	const float PingOffset = Scoreboard.x + Scoreboard.w - PingLength - 20.0f;
	const float CountryOffset = PingOffset - CountryLength;
	const float ClanOffset = NameOffset + NameLength + 5.0f;
	const float ClanLength = CountryOffset - ClanOffset - 5.0f;

	// EXACT headlines from scoreboard
	const float HeadlineFontsize = 22.0f;
	CUIRect Headline;
	Scoreboard.HSplitTop(HeadlineFontsize * 2.0f, &Headline, &Scoreboard);
	const float HeadlineY = Headline.y + Headline.h / 2.0f - HeadlineFontsize / 2.0f;
	const char *pScore = Localize("Rank"); // Changed from "Score"/"Time" to "Rank"
	TextRender()->Text(ScoreOffset + ScoreLength - TextRender()->TextWidth(HeadlineFontsize, pScore), HeadlineY, HeadlineFontsize, pScore);
	TextRender()->Text(NameOffset, HeadlineY, HeadlineFontsize, Localize("Name"));
	const char *pTimeLabel = Localize("Time"); // Added time column header
	TextRender()->Text(PingOffset + PingLength - TextRender()->TextWidth(HeadlineFontsize, pTimeLabel), HeadlineY, HeadlineFontsize, pTimeLabel);

	// EXACT player rendering loop from scoreboard, but with map times data
	int CountRendered = 0;

	char aBuf[64];

	// Simplified loop - no teams, no dead/alive, just records
	for(int i = 0; i < NumRecords && i < 16; i++) // Max 16 like scoreboard
	{
		const SMapTimeRecord *pRecord = GameClient()->m_MapTimes.GetRecord(i);
		if(!pRecord)
			break;

		CUIRect RowAndSpacing, Row;
		Scoreboard.HSplitTop(LineHeight + Spacing, &RowAndSpacing, &Scoreboard);
		RowAndSpacing.HSplitTop(LineHeight, &Row, nullptr);

		// EXACT background logic from scoreboard but with podium colors
		ColorRGBA BackgroundColor;
		if(pRecord->m_Rank == 1) // Gold
			BackgroundColor = ColorRGBA(1.0f, 0.84f, 0.0f, 0.5f);
		else if(pRecord->m_Rank == 2) // Silver
			BackgroundColor = ColorRGBA(0.75f, 0.75f, 0.75f, 0.5f);
		else if(pRecord->m_Rank == 3) // Bronze
			BackgroundColor = ColorRGBA(0.8f, 0.52f, 0.25f, 0.5f);
		else if(i % 2 == 0)
			BackgroundColor = ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f);
		else
			BackgroundColor = ColorRGBA(0.1f, 0.1f, 0.1f, 0.25f);

		RowAndSpacing.Draw(BackgroundColor, IGraphics::CORNER_ALL, RoundRadius);

		// EXACT text color logic
		ColorRGBA TextColor = TextRender()->DefaultTextColor();
		if(pRecord->m_Rank == 1)
			TextColor = ColorRGBA(1.0f, 0.84f, 0.0f, 1.0f);
		else if(pRecord->m_Rank == 2)
			TextColor = ColorRGBA(0.75f, 0.75f, 0.75f, 1.0f);
		else if(pRecord->m_Rank == 3)
			TextColor = ColorRGBA(0.8f, 0.52f, 0.25f, 1.0f);
		
		TextRender()->TextColor(TextColor);

		// Rank instead of score
		str_format(aBuf, sizeof(aBuf), "%d", pRecord->m_Rank);
		TextRender()->Text(ScoreOffset + ScoreLength - TextRender()->TextWidth(FontSize, aBuf), Row.y + (Row.h - FontSize) / 2.0f, FontSize, aBuf);

		// Skip tee rendering - we don't have tees for map times

		// EXACT name rendering from scoreboard
		{
			CTextCursor Cursor;
			TextRender()->SetCursor(&Cursor, NameOffset, Row.y + (Row.h - FontSize) / 2.0f, FontSize, TEXTFLAG_RENDER | TEXTFLAG_ELLIPSIS_AT_END);
			Cursor.m_LineWidth = NameLength;
			TextRender()->TextEx(&Cursor, pRecord->m_aPlayerName);
		}

		// Time in ping position (right-aligned)
		{
			char aFormattedTime[32];
			GameClient()->m_MapTimes.FormatTime(aFormattedTime, sizeof(aFormattedTime), pRecord->m_aTime);
			TextRender()->Text(PingOffset + PingLength - TextRender()->TextWidth(FontSize, aFormattedTime), Row.y + (Row.h - FontSize) / 2.0f, FontSize, aFormattedTime);
		}

		// Reset color
		TextRender()->TextColor(TextRender()->DefaultTextColor());

		CountRendered++;
	}
}

void CMapTimesMenu::OnRender()
{
	if(!IsActive())
		return;

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	// EXACT dimensions and layout from scoreboard OnRender
	const float Width = 400.0f * 3.0f;
	const float Height = 650.0f;
	const float w = Width;
	const float h = Height;

	const float TitleHeight = 60.0f;
	const float x = Graphics()->ScreenWidth() / 2.0f - w / 2.0f;
	const float y = 150.0f;

	CUIRect Scoreboard = {x, y, w, h};
	
	// EXACT background from scoreboard
	Scoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 15.0f);

	// EXACT title split
	CUIRect TitleBar, MainBody;
	Scoreboard.HSplitTop(TitleHeight, &TitleBar, &MainBody);
	
	// Render title
	RenderTitle(TitleBar, "Map Times Leaderboard");
	
	// EXACT margins from scoreboard
	MainBody.VMargin(10.0f, &MainBody);
	MainBody.VMargin(10.0f, &MainBody);
	
	// Render map times using scoreboard logic
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
	return IsActive();
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
