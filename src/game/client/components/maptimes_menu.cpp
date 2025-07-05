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

void CMapTimesMenu::CalculateLayout(int NumEntries, CMapTimesRenderState &State)
{
	// Adapt layout based on number of entries (similar to scoreboard)
	if(NumEntries <= 8)
	{
		State.m_LineHeight = 60.0f;
		State.m_FontSize = 24.0f;
		State.m_RoundRadius = 10.0f;
		State.m_Spacing = 16.0f;
	}
	else if(NumEntries <= 12)
	{
		State.m_LineHeight = 50.0f;
		State.m_FontSize = 22.0f;
		State.m_RoundRadius = 10.0f;
		State.m_Spacing = 5.0f;
	}
	else if(NumEntries <= 16)
	{
		State.m_LineHeight = 40.0f;
		State.m_FontSize = 20.0f;
		State.m_RoundRadius = 5.0f;
		State.m_Spacing = 0.0f;
	}
	else
	{
		State.m_LineHeight = 30.0f;
		State.m_FontSize = 18.0f;
		State.m_RoundRadius = 5.0f;
		State.m_Spacing = 0.0f;
	}
}

void CMapTimesMenu::RenderBackground(CUIRect Background)
{
	// Dark semi-transparent background like scoreboard
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.8f);
	Graphics()->QuadsBegin();
	IGraphics::CQuadItem BackgroundQuad(Background.x, Background.y, Background.w, Background.h);
	Graphics()->QuadsDraw(&BackgroundQuad, 1);
	Graphics()->QuadsEnd();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CMapTimesMenu::RenderTitle(CUIRect TitleBar, const char *pTitle)
{
	// Title background (darker than main background)
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.9f);
	Graphics()->QuadsBegin();
	IGraphics::CQuadItem TitleQuad(TitleBar.x, TitleBar.y, TitleBar.w, TitleBar.h);
	Graphics()->QuadsDraw(&TitleQuad, 1);
	Graphics()->QuadsEnd();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	
	// Title text
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	
	char aFullTitle[128];
	const char *pCurrentMap = Client()->GetCurrentMap();
	if(pCurrentMap && pCurrentMap[0] != '\0')
		str_format(aFullTitle, sizeof(aFullTitle), "%s - %s", pTitle, pCurrentMap);
	else
		str_copy(aFullTitle, pTitle, sizeof(aFullTitle));
	
	float TitleWidth = TextRender()->TextWidth(28.0f, aFullTitle, -1, -1.0f);
	float TitleX = TitleBar.x + TitleBar.w / 2.0f - TitleWidth / 2.0f;
	float TitleY = TitleBar.y + TitleBar.h / 2.0f - 14.0f;
	
	TextRender()->Text(TitleX, TitleY, 28.0f, aFullTitle);
}

void CMapTimesMenu::RenderEntry(CUIRect Entry, int Rank, const char *pName, const char *pTime, int Index, CMapTimesRenderState &State)
{
	const float RankWidth = 80.0f;
	const float TimeWidth = 140.0f;
	const float Padding = 15.0f;
	
	// Background color based on rank (like scoreboard alternating + podium colors)
	ColorRGBA BackgroundColor;
	if(Rank == 1) // 1st place - Gold
		BackgroundColor = ColorRGBA(1.0f, 0.84f, 0.0f, 0.4f);
	else if(Rank == 2) // 2nd place - Silver  
		BackgroundColor = ColorRGBA(0.75f, 0.75f, 0.75f, 0.4f);
	else if(Rank == 3) // 3rd place - Bronze
		BackgroundColor = ColorRGBA(0.8f, 0.52f, 0.25f, 0.4f);
	else if(Index % 2 == 0)
		BackgroundColor = ColorRGBA(0.0f, 0.0f, 0.0f, 0.3f); // Even rows
	else
		BackgroundColor = ColorRGBA(0.1f, 0.1f, 0.1f, 0.3f); // Odd rows
	
	// Draw entry background with rounded corners (like scoreboard)
	Graphics()->SetColor(BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, BackgroundColor.a);
	Graphics()->QuadsBegin();
	Graphics()->SetColorVertex(0, BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, BackgroundColor.a);
	Graphics()->SetColorVertex(1, BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, BackgroundColor.a);
	Graphics()->SetColorVertex(2, BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, BackgroundColor.a);
	Graphics()->SetColorVertex(3, BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, BackgroundColor.a);
	IGraphics::CQuadItem EntryQuad(Entry.x + 2.0f, Entry.y + State.m_Spacing / 2.0f, Entry.w - 4.0f, Entry.h - State.m_Spacing);
	Graphics()->QuadsDrawTL(&EntryQuad, 1);
	Graphics()->QuadsEnd();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	
	// Text color based on rank
	ColorRGBA TextColor;
	if(Rank == 1) // 1st place - Gold
		TextColor = ColorRGBA(1.0f, 0.84f, 0.0f, 1.0f);
	else if(Rank == 2) // 2nd place - Silver
		TextColor = ColorRGBA(0.75f, 0.75f, 0.75f, 1.0f);
	else if(Rank == 3) // 3rd place - Bronze
		TextColor = ColorRGBA(0.8f, 0.52f, 0.25f, 1.0f);
	else
		TextColor = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, TextColor.a);
	
	// Calculate vertical center position for text
	float TextY = Entry.y + Entry.h / 2.0f - State.m_FontSize / 2.0f;
	
	// Render rank
	char aRankText[16];
	if(Rank == 1)
		str_format(aRankText, sizeof(aRankText), "🥇 %d.", Rank);
	else if(Rank == 2)
		str_format(aRankText, sizeof(aRankText), "🥈 %d.", Rank);
	else if(Rank == 3)
		str_format(aRankText, sizeof(aRankText), "🥉 %d.", Rank);
	else
		str_format(aRankText, sizeof(aRankText), "%d.", Rank);
	
	float RankX = Entry.x + Padding;
	TextRender()->Text(RankX, TextY, State.m_FontSize, aRankText);
	
	// Render player name (centered)
	float NameX = Entry.x + RankWidth + Padding;
	float NameMaxWidth = Entry.w - RankWidth - TimeWidth - 3 * Padding;
	TextRender()->Text(NameX, TextY, State.m_FontSize, pName, NameMaxWidth);
	
	// Render time (right aligned)
	float TimeWidth = TextRender()->TextWidth(State.m_FontSize, pTime, -1, -1.0f);
	float TimeX = Entry.x + Entry.w - TimeWidth - Padding;
	TextRender()->Text(TimeX, TextY, State.m_FontSize, pTime);
	
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CMapTimesMenu::OnRender()
{
	if(!IsActive())
		return;

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	// Setup screen coordinates
	Graphics()->MapScreen(0, 0, Graphics()->ScreenWidth(), Graphics()->ScreenHeight());

	// Calculate leaderboard dimensions (like scoreboard)
	float LeaderboardWidth = minimum(Graphics()->ScreenWidth() - 60.0f, 800.0f);
	float LeaderboardHeight = minimum(Graphics()->ScreenHeight() - 120.0f, 600.0f);
	float TitleHeight = 60.0f;
	
	// Center on screen
	float LeaderboardX = Graphics()->ScreenWidth() / 2.0f - LeaderboardWidth / 2.0f;
	float LeaderboardY = Graphics()->ScreenHeight() / 2.0f - LeaderboardHeight / 2.0f;
	
	// Main leaderboard rect
	CUIRect MainRect;
	MainRect.x = LeaderboardX;
	MainRect.y = LeaderboardY;
	MainRect.w = LeaderboardWidth;
	MainRect.h = LeaderboardHeight;
	
	// Render main background (like scoreboard)
	RenderBackground(MainRect);
	
	// Split into title and content
	CUIRect TitleRect, ContentRect;
	MainRect.HSplitTop(TitleHeight, &TitleRect, &ContentRect);
	
	// Render title
	RenderTitle(TitleRect, "Map Times Leaderboard");
	
	// Add margin to content (like scoreboard padding)
	ContentRect.Margin(15.0f, &ContentRect);
	
	// Render map times content
	CMapTimesRenderState RenderState;
	RenderMapTimes(ContentRect, RenderState);
	
	// Instructions at the bottom (like scoreboard style)
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.7f);
	
	const char *pInstructions = "Press ESC or release assigned key to close";
	float InstructionsWidth = TextRender()->TextWidth(16.0f, pInstructions, -1, -1.0f);
	float InstructionsX = Graphics()->ScreenWidth() / 2.0f - InstructionsWidth / 2.0f;
	float InstructionsY = Graphics()->ScreenHeight() - 40.0f;
	
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
	// Don't handle direct input anymore - control is now via console command (+map_times)
	// Only handle ESC key to close the menu
	if(IsActive() && Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
	{
		SetActive(false);
		return true;
	}

	return IsActive(); // Consume all input when active, but don't process keys for opening
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

void CMapTimesMenu::RenderMapTimes(CUIRect MapTimes, CMapTimesRenderState &State)
{
	// Get data from MapTimes component
	if(!GameClient()->m_MapTimes.HasValidData())
	{
		// Show loading message in scoreboard style
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.8f);
		
		const char *pLoadingText = "Loading map times...";
		float LoadingWidth = TextRender()->TextWidth(24.0f, pLoadingText, -1, -1.0f);
		float LoadingX = MapTimes.x + MapTimes.w / 2.0f - LoadingWidth / 2.0f;
		float LoadingY = MapTimes.y + MapTimes.h / 2.0f - 12.0f;
		
		TextRender()->Text(LoadingX, LoadingY, 24.0f, pLoadingText);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		return;
	}
	
	// Get map times data (we'll use the same data structure as the existing MapTimes component)
	const int NumEntries = 10; // Top 10
	CalculateLayout(NumEntries, State);
	
	// Create column headers (like scoreboard)
	const float HeadlineFontsize = 22.0f;
	CUIRect Headline;
	MapTimes.HSplitTop(HeadlineFontsize * 2.0f, &Headline, &MapTimes);
	
	const float RankWidth = 80.0f;
	const float TimeWidth = 140.0f;
	const float Padding = 15.0f;
	
	// Render column headers
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.9f);
	
	const float HeadlineY = Headline.y + Headline.h / 2.0f - HeadlineFontsize / 2.0f;
	
	// Rank header
	TextRender()->Text(Headline.x + Padding, HeadlineY, HeadlineFontsize, "Rank");
	
	// Name header (centered)
	const char *pNameLabel = "Player Name";
	float NameX = Headline.x + RankWidth + Padding;
	TextRender()->Text(NameX, HeadlineY, HeadlineFontsize, pNameLabel);
	
	// Time header (right aligned)
	const char *pTimeLabel = "Time";
	float TimeLabelWidth = TextRender()->TextWidth(HeadlineFontsize, pTimeLabel, -1, -1.0f);
	float TimeX = Headline.x + Headline.w - TimeLabelWidth - Padding;
	TextRender()->Text(TimeX, HeadlineY, HeadlineFontsize, pTimeLabel);
	
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	
	// Render entries
	CUIRect EntryRect = MapTimes;
	EntryRect.h = State.m_LineHeight;
	
	// Get real data from MapTimes component
	const int NumRecords = GameClient()->m_MapTimes.GetNumRecords();
	for(int i = 0; i < NumRecords && i < NumEntries; i++)
	{
		if(EntryRect.y + EntryRect.h > MapTimes.y + MapTimes.h)
			break; // Don't render outside bounds
		
		const SMapTimeRecord *pRecord = GameClient()->m_MapTimes.GetRecord(i);
		if(!pRecord)
			break;
		
		// Format time to show only 2 decimals (like the existing implementation)
		char aFormattedTime[32];
		GameClient()->m_MapTimes.FormatTime(aFormattedTime, sizeof(aFormattedTime), pRecord->m_aTime);
		
		RenderEntry(EntryRect, pRecord->m_Rank, pRecord->m_aPlayerName, aFormattedTime, i, State);
		EntryRect.y += State.m_LineHeight;
	}
}
