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
	// Title background (like scoreboard title)
	TitleBar.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 15.0f);
	
	const float TitleFontSize = 40.0f;
	char aFullTitle[128];
	const char *pCurrentMap = Client()->GetCurrentMap();
	if(pCurrentMap && pCurrentMap[0] != '\0')
		str_format(aFullTitle, sizeof(aFullTitle), "%s - %s", pTitle, pCurrentMap);
	else
		str_copy(aFullTitle, pTitle, sizeof(aFullTitle));
	
	// Center the title like scoreboard
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
		// Show loading message
		SLabelProperties Props;
		Props.m_MaxWidth = Scoreboard.w;
		Props.m_EllipsisAtEnd = true;
		Ui()->DoLabel(&Scoreboard, "Loading map times...", 24.0f, TEXTALIGN_MC, Props);
		return;
	}

	const int NumRecords = GameClient()->m_MapTimes.GetNumRecords();
	if(NumRecords == 0)
	{
		// Show no records message
		SLabelProperties Props;
		Props.m_MaxWidth = Scoreboard.w;
		Props.m_EllipsisAtEnd = true;
		Ui()->DoLabel(&Scoreboard, "No records available", 24.0f, TEXTALIGN_MC, Props);
		return;
	}

	// Adaptive layout based on number of records (like scoreboard)
	float LineHeight, FontSize, RoundRadius, Spacing;
	if(NumRecords <= 8)
	{
		LineHeight = 60.0f;
		FontSize = 24.0f;
		RoundRadius = 10.0f;
		Spacing = 16.0f;
	}
	else if(NumRecords <= 12)
	{
		LineHeight = 50.0f;
		FontSize = 22.0f;
		RoundRadius = 5.0f;
		Spacing = 5.0f;
	}
	else if(NumRecords <= 16)
	{
		LineHeight = 40.0f;
		FontSize = 20.0f;
		RoundRadius = 5.0f;
		Spacing = 0.0f;
	}
	else
	{
		LineHeight = 30.0f;
		FontSize = 18.0f;
		RoundRadius = 2.0f;
		Spacing = 0.0f;
	}

	// Column layout (like scoreboard)
	const float RankOffset = Scoreboard.x + 40.0f;
	const float RankLength = 80.0f;
	const float TimeLength = TextRender()->TextWidth(FontSize, "00:00:00");
	const float TimeOffset = Scoreboard.x + Scoreboard.w - TimeLength - 20.0f;
	const float NameOffset = RankOffset + RankLength;
	const float NameLength = TimeOffset - NameOffset - 20.0f;

	// Render column headers (like scoreboard)
	const float HeadlineFontsize = 22.0f;
	CUIRect Headline;
	Scoreboard.HSplitTop(HeadlineFontsize * 2.0f, &Headline, &Scoreboard);
	const float HeadlineY = Headline.y + Headline.h / 2.0f - HeadlineFontsize / 2.0f;
	
	TextRender()->Text(RankOffset, HeadlineY, HeadlineFontsize, Localize("Rank"));
	TextRender()->Text(NameOffset, HeadlineY, HeadlineFontsize, Localize("Name"));
	const char *pTimeLabel = Localize("Time");
	TextRender()->Text(TimeOffset + TimeLength - TextRender()->TextWidth(HeadlineFontsize, pTimeLabel), HeadlineY, HeadlineFontsize, pTimeLabel);

	// Render entries
	const int MaxDisplayed = minimum(NumRecords, 16); // Don't show more than 16 entries
	for(int i = 0; i < MaxDisplayed; i++)
	{
		const SMapTimeRecord *pRecord = GameClient()->m_MapTimes.GetRecord(i);
		if(!pRecord)
			break;

		CUIRect RowAndSpacing, Row;
		Scoreboard.HSplitTop(LineHeight + Spacing, &RowAndSpacing, &Scoreboard);
		RowAndSpacing.HSplitTop(LineHeight, &Row, nullptr);

		// Background color based on rank (like scoreboard)
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

		// Draw row background
		RowAndSpacing.Draw(BackgroundColor, IGraphics::CORNER_ALL, RoundRadius);

		// Text color for podium places
		ColorRGBA TextColor = TextRender()->DefaultTextColor();
		if(pRecord->m_Rank == 1)
			TextColor = ColorRGBA(1.0f, 0.84f, 0.0f, 1.0f);
		else if(pRecord->m_Rank == 2)
			TextColor = ColorRGBA(0.75f, 0.75f, 0.75f, 1.0f);
		else if(pRecord->m_Rank == 3)
			TextColor = ColorRGBA(0.8f, 0.52f, 0.25f, 1.0f);
		
		TextRender()->TextColor(TextColor);

		// Render rank with medal emojis
		char aRankText[16];
		if(pRecord->m_Rank == 1)
			str_format(aRankText, sizeof(aRankText), "🥇 %d", pRecord->m_Rank);
		else if(pRecord->m_Rank == 2)
			str_format(aRankText, sizeof(aRankText), "🥈 %d", pRecord->m_Rank);
		else if(pRecord->m_Rank == 3)
			str_format(aRankText, sizeof(aRankText), "🥉 %d", pRecord->m_Rank);
		else
			str_format(aRankText, sizeof(aRankText), "%d", pRecord->m_Rank);
		
		TextRender()->Text(RankOffset, Row.y + (Row.h - FontSize) / 2.0f, FontSize, aRankText);

		// Render name with ellipsis
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, NameOffset, Row.y + (Row.h - FontSize) / 2.0f, FontSize, TEXTFLAG_RENDER | TEXTFLAG_ELLIPSIS_AT_END);
		Cursor.m_LineWidth = NameLength;
		TextRender()->TextEx(&Cursor, pRecord->m_aPlayerName);

		// Format and render time
		char aFormattedTime[32];
		GameClient()->m_MapTimes.FormatTime(aFormattedTime, sizeof(aFormattedTime), pRecord->m_aTime);
		TextRender()->Text(TimeOffset + TimeLength - TextRender()->TextWidth(FontSize, aFormattedTime), Row.y + (Row.h - FontSize) / 2.0f, FontSize, aFormattedTime);

		// Reset text color
		TextRender()->TextColor(TextRender()->DefaultTextColor());
	}
}

void CMapTimesMenu::OnRender()
{
	if(!IsActive())
		return;

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	// Calculate scoreboard dimensions (exactly like scoreboard)
	const float Width = 400.0f * 3.0f;
	const float Height = 650.0f;
	const float w = Width;
	const float h = Height;

	const float TitleHeight = 60.0f;
	const float x = Graphics()->ScreenWidth() / 2.0f - w / 2.0f;
	const float y = 150.0f;

	CUIRect Scoreboard = {x, y, w, h};
	
	// Main scoreboard background (like scoreboard)
	Scoreboard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 15.0f);

	// Split title and content
	CUIRect TitleBar, MainBody;
	Scoreboard.HSplitTop(TitleHeight, &TitleBar, &MainBody);
	
	// Render title
	RenderTitle(TitleBar, "Map Times Leaderboard");
	
	// Add margins like scoreboard
	MainBody.VMargin(10.0f, &MainBody);
	MainBody.VMargin(10.0f, &MainBody);
	
	// Render the map times content
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
	return true; // Consume cursor movement when active
}

bool CMapTimesMenu::OnInput(const IInput::CEvent &Event)
{
	// Handle ESC key to close the menu
	if(IsActive() && Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
	{
		SetActive(false);
		return true;
	}

	return IsActive(); // Consume all input when active
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
