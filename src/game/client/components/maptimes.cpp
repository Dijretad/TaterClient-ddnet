/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "maptimes.h"

#include <base/log.h>
#include <base/system.h>
#include <engine/client.h>
#include <engine/external/json-parser/json.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/http.h>
#include <engine/textrender.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/components/chat.h>

CMapTimes::CMapTimes()
{
	m_NumRecords = 0;
	m_State = STATE_IDLE;
	m_aCurrentMap[0] = '\0';
	m_LastRequestTime = 0;
	m_LastUpdateTime = 0;
	m_ContainersValid = false;
}

void CMapTimes::OnInit()
{
	ResetTextContainers();
	
	// Register console command
	Console()->Register("show_top_10", "", CFGFLAG_CLIENT, ConShowTop10, this, "Show top 10 records for current map in chat");
}

void CMapTimes::OnReset()
{
	m_State = STATE_IDLE;
	m_NumRecords = 0;
	m_aCurrentMap[0] = '\0';
	m_LastRequestTime = 0;
	m_LastUpdateTime = 0;
	ResetTextContainers();
	
	if(m_pRequest)
	{
		m_pRequest->Abort();
		m_pRequest = nullptr;
	}
}

void CMapTimes::OnMapLoad()
{
	const char *pCurrentMap = Client()->GetCurrentMap();
	if(!pCurrentMap || str_comp(m_aCurrentMap, pCurrentMap) == 0)
		return;
		
	str_copy(m_aCurrentMap, pCurrentMap);
	RequestMapTimes(pCurrentMap);
}

void CMapTimes::RequestMapTimes(const char *pMapName)
{
	if(!pMapName || pMapName[0] == '\0')
		return;
		
	// Don't spam requests
	int64_t CurrentTime = time_get();
	if(CurrentTime - m_LastRequestTime < time_freq() * 10) // 10 seconds cooldown
		return;
		
	m_LastRequestTime = CurrentTime;
	m_State = STATE_LOADING;
	m_NumRecords = 0;
	ResetTextContainers();
	
	if(m_pRequest)
	{
		m_pRequest->Abort();
		m_pRequest = nullptr;
	}
	
	char aUrl[512];
	char aEscapedMapName[256];
	EscapeUrl(aEscapedMapName, sizeof(aEscapedMapName), pMapName);
	str_format(aUrl, sizeof(aUrl), "https://www.ravenkog.com/api/maps?mapName=%s", aEscapedMapName);
	
	m_pRequest = HttpGet(aUrl);
	m_pRequest->Timeout(CTimeout{10000, 0, 500, 10});
	m_pRequest->WriteToMemory();
	m_pRequest->LogProgress(HTTPLOG::FAILURE);
	GameClient()->Http()->Run(m_pRequest);
	
	log_debug("maptimes", "Requesting map times for '%s' from %s", pMapName, aUrl);
}

void CMapTimes::ParseResponse(const char *pJson)
{
	json_value *pRoot = json_parse(pJson, str_length(pJson));
	if(!pRoot)
	{
		log_error("maptimes", "Failed to parse JSON response");
		m_State = STATE_ERROR;
		return;
	}
	
	m_NumRecords = 0;
	
	// Find the top100 array
	const json_value *pTop100 = nullptr;
	if(pRoot->type == json_object)
	{
		for(unsigned i = 0; i < pRoot->u.object.length; i++)
		{
			if(str_comp(pRoot->u.object.values[i].name, "top100") == 0)
			{
				pTop100 = pRoot->u.object.values[i].value;
				break;
			}
		}
	}
	
	if(!pTop100 || pTop100->type != json_array)
	{
		log_error("maptimes", "No top100 array found in response");
		json_value_free(pRoot);
		m_State = STATE_ERROR;
		return;
	}
	
	// Parse the first 10 records
	int RecordCount = minimum((int)pTop100->u.array.length, (int)MAX_TOP_RECORDS);
	for(int i = 0; i < RecordCount; i++)
	{
		const json_value *pRecord = pTop100->u.array.values[i];
		if(!pRecord || pRecord->type != json_object)
			continue;
			
		SMapTimeRecord &Record = m_aTopRecords[m_NumRecords];
		
		// Parse player name
		for(unsigned j = 0; j < pRecord->u.object.length; j++)
		{
			const char *pKey = pRecord->u.object.values[j].name;
			const json_value *pValue = pRecord->u.object.values[j].value;
			
			if(str_comp(pKey, "playerName") == 0 && pValue->type == json_string)
			{
				str_copy(Record.m_aPlayerName, pValue->u.string.ptr, sizeof(Record.m_aPlayerName));
			}
			else if(str_comp(pKey, "time") == 0 && pValue->type == json_object)
			{
				// Find the "value" field in the time object
				for(unsigned k = 0; k < pValue->u.object.length; k++)
				{
					if(str_comp(pValue->u.object.values[k].name, "value") == 0 && 
					   pValue->u.object.values[k].value->type == json_string)
					{
						str_copy(Record.m_aTime, pValue->u.object.values[k].value->u.string.ptr, sizeof(Record.m_aTime));
						break;
					}
				}
			}
			else if(str_comp(pKey, "rank") == 0 && pValue->type == json_integer)
			{
				Record.m_Rank = (int)pValue->u.integer;
			}
		}
		
		if(Record.m_aPlayerName[0] != '\0' && Record.m_aTime[0] != '\0')
		{
			m_NumRecords++;
		}
	}
	
	json_value_free(pRoot);
	
	if(m_NumRecords > 0)
	{
		m_State = STATE_DONE;
		log_debug("maptimes", "Successfully parsed %d records", m_NumRecords);
	}
	else
	{
		m_State = STATE_ERROR;
		log_error("maptimes", "No valid records found");
	}
}

void CMapTimes::Update()
{
	if(m_pRequest && m_pRequest->Done())
	{
		if(m_pRequest->State() == EHttpState::DONE && m_pRequest->StatusCode() == 200)
		{
			unsigned char *pResult;
			size_t ResultLength;
			m_pRequest->Result(&pResult, &ResultLength);
			
			if(pResult && ResultLength > 0)
			{
				// Add null terminator
				char *pJsonString = (char *)malloc(ResultLength + 1);
				mem_copy(pJsonString, pResult, ResultLength);
				pJsonString[ResultLength] = '\0';
				
				ParseResponse(pJsonString);
				free(pJsonString);
			}
			else
			{
				m_State = STATE_ERROR;
			}
		}
		else
		{
			log_error("maptimes", "HTTP request failed with status %d", m_pRequest->StatusCode());
			m_State = STATE_ERROR;
		}
		
		m_pRequest = nullptr;
		m_LastUpdateTime = time_get();
	}
	
	// Update text containers if needed
	if(m_State == STATE_DONE && !m_ContainersValid)
	{
		UpdateTextContainers();
	}
}

void CMapTimes::ResetTextContainers()
{
	for(int i = 0; i < MAX_TOP_RECORDS; i++)
	{
		m_aPlayerNameContainers[i].Reset();
		m_aTimeContainers[i].Reset();
	}
	m_HeaderContainer.Reset();
	m_ContainersValid = false;
}

void CMapTimes::UpdateTextContainers()
{
	ResetTextContainers();
	
	const float FontSize = 10.0f;
	const float HeaderFontSize = 12.0f;
	
	// Create header container
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, 0, 0, HeaderFontSize, TEXTFLAG_RENDER);
	TextRender()->RecreateTextContainer(m_HeaderContainer, &Cursor, "Top 5 Records");
	
	// Create containers for each record
	for(int i = 0; i < m_NumRecords; i++)
	{
		const SMapTimeRecord &Record = m_aTopRecords[i];
		
		// Player name
		TextRender()->SetCursor(&Cursor, 0, 0, FontSize, TEXTFLAG_RENDER);
		TextRender()->RecreateTextContainer(m_aPlayerNameContainers[i], &Cursor, Record.m_aPlayerName);
		
		// Time (remove microseconds for cleaner display)
		char aCleanTime[32];
		str_copy(aCleanTime, Record.m_aTime, sizeof(aCleanTime));
		const char *pDot = str_find(aCleanTime, ".");
		if(pDot && str_length(pDot) > 3)
		{
			// Find position and truncate
			int DotPos = pDot - aCleanTime;
			if(DotPos + 3 < (int)sizeof(aCleanTime))
				aCleanTime[DotPos + 3] = '\0'; // Keep only 2 decimal places
		}
		
		TextRender()->SetCursor(&Cursor, 0, 0, FontSize, TEXTFLAG_RENDER);
		TextRender()->RecreateTextContainer(m_aTimeContainers[i], &Cursor, aCleanTime);
	}
	
	m_ContainersValid = true;
}

void CMapTimes::RenderMapTimes(float x, float y, float w, float h)
{
	if(m_State != STATE_DONE || m_NumRecords == 0)
		return;
		
	if(!m_ContainersValid)
		UpdateTextContainers();
	
	// Background
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.6f);
	Graphics()->DrawRectExt(x, y, w, h, 5.0f, IGraphics::CORNER_ALL);
	Graphics()->QuadsEnd();
	
	// Header
	const float HeaderHeight = 20.0f;
	const float RowHeight = 16.0f;
	const float Padding = 5.0f;
	
	if(m_HeaderContainer.Valid())
	{
		Graphics()->TextureClear();
		ColorRGBA HeaderColor(1.0f, 1.0f, 1.0f, 1.0f);
		ColorRGBA HeaderOutline(0.0f, 0.0f, 0.0f, 0.5f);
		TextRender()->RenderTextContainer(m_HeaderContainer, HeaderColor, HeaderOutline, x + Padding, y + Padding);
	}
	
	// Records
	float CurrentY = y + Padding + HeaderHeight;
	for(int i = 0; i < m_NumRecords && i < MAX_TOP_RECORDS; i++)
	{
		// Rank
		char aRankStr[8];
		str_format(aRankStr, sizeof(aRankStr), "%d.", i + 1);
		
		ColorRGBA TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		ColorRGBA OutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
		
		// Different colors for top 3
		if(i == 0)
			TextColor = ColorRGBA(1.0f, 0.84f, 0.0f, 1.0f); // Gold
		else if(i == 1)
			TextColor = ColorRGBA(0.75f, 0.75f, 0.75f, 1.0f); // Silver
		else if(i == 2)
			TextColor = ColorRGBA(0.8f, 0.5f, 0.2f, 1.0f); // Bronze
		
		// Rank number
		TextRender()->Text(x + Padding, CurrentY, 10.0f, aRankStr, -1.0f);
		
		// Player name
		if(m_aPlayerNameContainers[i].Valid())
		{
			TextRender()->RenderTextContainer(m_aPlayerNameContainers[i], TextColor, OutlineColor, x + Padding + 20.0f, CurrentY);
		}
		
		// Time
		if(m_aTimeContainers[i].Valid())
		{
			TextRender()->RenderTextContainer(m_aTimeContainers[i], TextColor, OutlineColor, x + w - Padding - 60.0f, CurrentY);
		}
		
		CurrentY += RowHeight;
	}
}

void CMapTimes::RenderMapTimesTab(float x, float y)
{
	if(!g_Config.m_ClShowhudMapTimes || !HasValidData())
		return;

	// Use configurable text size (default 50% = half size)
	const float BaseFontSize = 6.0f; // Reduced from 12.0f
	const float FontSize = BaseFontSize * (g_Config.m_ClMapTimesTextSize / 100.0f);
	const float LineHeight = FontSize + 4.0f;
	const float HeaderHeight = FontSize + 10.0f;
	
	// Background
	float Width = 250.0f;
	float Height = HeaderHeight + (m_NumRecords * LineHeight) + 10.0f;
	
	// Use the current graphics context (scoreboard has already set up MapScreen)
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.6f);
	IGraphics::CQuadItem Quad(x, y, Width, Height);
	Graphics()->QuadsBegin();
	Graphics()->QuadsDraw(&Quad, 1);
	Graphics()->QuadsEnd();
	
	// Header
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	
	char aHeader[64];
	str_format(aHeader, sizeof(aHeader), "Top %d Records - %s", m_NumRecords, m_aCurrentMap);
	
	TextRender()->Text(x + 5.0f, y + 5.0f, FontSize + 2.0f, aHeader);
	
	// Records
	for(int i = 0; i < m_NumRecords; i++)
	{
		const SMapTimeRecord &Record = m_aTopRecords[i];
		float RecordY = y + HeaderHeight + (i * LineHeight);
		
		// Rank color
		if(i == 0)
			TextRender()->TextColor(1.0f, 0.8f, 0.0f, 1.0f); // Gold
		else if(i == 1)
			TextRender()->TextColor(0.8f, 0.8f, 0.8f, 1.0f); // Silver
		else if(i == 2)
			TextRender()->TextColor(0.8f, 0.5f, 0.2f, 1.0f); // Bronze
		else
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f); // White
		
		// Format time to show only 2 decimal places
		char aFormattedTime[32];
		FormatTime(aFormattedTime, sizeof(aFormattedTime), Record.m_aTime);
		
		// Format: "1. PlayerName - 1:23.45"
		char aLine[128];
		str_format(aLine, sizeof(aLine), "%d. %s - %s", i + 1, Record.m_aPlayerName, aFormattedTime);
		
		TextRender()->Text(x + 10.0f, RecordY, FontSize, aLine);
	}
	
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f); // Reset color
}

void CMapTimes::FormatTime(char *pBuffer, int BufferSize, const char *pTimeString)
{
	// Input format: "00:20:02.440000"
	// Output format: "00:20:02.44"
	
	if(!pTimeString || !pTimeString[0])
	{
		str_copy(pBuffer, "", BufferSize);
		return;
	}
	
	// Find the decimal point
	const char *pDecimal = str_find(pTimeString, ".");
	if(!pDecimal)
	{
		// No decimal point, just copy as is
		str_copy(pBuffer, pTimeString, BufferSize);
		return;
	}
	
	// Calculate length up to decimal point + 3 characters (decimal + 2 digits)
	int BaseLength = pDecimal - pTimeString;
	int TotalLength = minimum(BaseLength + 3, (int)str_length(pTimeString));
	TotalLength = minimum(TotalLength, BufferSize - 1);
	
	str_copy(pBuffer, pTimeString, TotalLength + 1);
}

void CMapTimes::ShowTop10InChat()
{
	if(!HasValidData())
	{
		GameClient()->m_Chat.AddLine(CChat::TYPE_ALL, 0, "No map times data available. Wait for the data to load or ensure you're on a valid map.");
		return;
	}
	
	// Header message
	char aHeaderMsg[128];
	str_format(aHeaderMsg, sizeof(aHeaderMsg), "=== Top %d Records for %s ===", m_NumRecords, m_aCurrentMap);
	GameClient()->m_Chat.AddLine(CChat::TYPE_ALL, 0, aHeaderMsg);
	
	// Display each record
	for(int i = 0; i < m_NumRecords; i++)
	{
		const SMapTimeRecord &Record = m_aTopRecords[i];
		
		// Format time to show only 2 decimal places
		char aFormattedTime[32];
		FormatTime(aFormattedTime, sizeof(aFormattedTime), Record.m_aTime);
		
		// Create the message line
		char aRecordMsg[256];
		str_format(aRecordMsg, sizeof(aRecordMsg), "%d. %s - %s", i + 1, Record.m_aPlayerName, aFormattedTime);
		
		// Add medal emojis for top 3
		char aFinalMsg[256];
		if(i == 0)
			str_format(aFinalMsg, sizeof(aFinalMsg), "🥇 %s", aRecordMsg);
		else if(i == 1)
			str_format(aFinalMsg, sizeof(aFinalMsg), "🥈 %s", aRecordMsg);
		else if(i == 2)
			str_format(aFinalMsg, sizeof(aFinalMsg), "🥉 %s", aRecordMsg);
		else
			str_copy(aFinalMsg, aRecordMsg, sizeof(aFinalMsg));
		
		GameClient()->m_Chat.AddLine(CChat::TYPE_ALL, 0, aFinalMsg);
	}
	
	// Footer message
	GameClient()->m_Chat.AddLine(CChat::TYPE_ALL, 0, "=========================");
}

void CMapTimes::ConShowTop10(IConsole::IResult *pResult, void *pUser)
{
	CMapTimes *pMapTimes = (CMapTimes *)pUser;
	
	// If no data available, try to request it first
	if(!pMapTimes->HasValidData())
	{
		const char *pCurrentMap = pMapTimes->Client()->GetCurrentMap();
		if(pCurrentMap && pCurrentMap[0] != '\0')
		{
			pMapTimes->GameClient()->m_Chat.AddLine(CChat::TYPE_ALL, 0, "Requesting map times data... Please wait and try again in a few seconds.");
			pMapTimes->RequestMapTimes(pCurrentMap);
			return;
		}
	}
	
	pMapTimes->ShowTop10InChat();
}

void CMapTimes::OnRender()
{
	Update();
	
	// Auto-request times when connecting to a server
	if(m_State == STATE_IDLE && Client()->State() == IClient::STATE_ONLINE)
	{
		const char *pCurrentMap = Client()->GetCurrentMap();
		if(pCurrentMap && pCurrentMap[0] != '\0' && str_comp(m_aCurrentMap, pCurrentMap) != 0)
		{
			OnMapLoad();
		}
	}
}

void CMapTimes::RenderMapTimesFullscreen(float x, float y)
{
	if(!HasValidData())
	{
		// Show loading or error message
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		
		const char *pMessage = "Loading map records...";
		if(m_State == STATE_ERROR)
			pMessage = "Failed to load map records.";
			
		float MessageWidth = TextRender()->TextWidth(24.0f, pMessage, -1, -1.0f);
		float MessageX = Graphics()->ScreenWidth() / 2.0f - MessageWidth / 2.0f;
		float MessageY = Graphics()->ScreenHeight() / 2.0f;
		
		TextRender()->Text(MessageX, MessageY, 24.0f, pMessage);
		return;
	}

	// Use larger font size for fullscreen view
	const float BaseFontSize = 16.0f;
	const float FontSize = BaseFontSize * (g_Config.m_ClMapTimesTextSize / 100.0f);
	const float LineHeight = FontSize + 8.0f;
	const float HeaderHeight = FontSize + 15.0f;
	
	// Larger background for fullscreen
	float Width = 600.0f;
	float Height = HeaderHeight + (m_NumRecords * LineHeight) + 20.0f;
	
	// Background with border
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.8f);
	IGraphics::CQuadItem BackgroundQuad(x, y, Width, Height);
	Graphics()->QuadsBegin();
	Graphics()->QuadsDraw(&BackgroundQuad, 1);
	Graphics()->QuadsEnd();
	
	// Border
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.3f);
	IGraphics::CQuadItem BorderQuad(x-2, y-2, Width+4, Height+4);
	Graphics()->QuadsBegin();
	Graphics()->QuadsDraw(&BorderQuad, 1);
	Graphics()->QuadsEnd();
	
	// Header
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	
	char aHeader[128];
	str_format(aHeader, sizeof(aHeader), "🏆 Top %d Records - %s", m_NumRecords, m_aCurrentMap);
	
	float HeaderWidth = TextRender()->TextWidth(FontSize + 4.0f, aHeader, -1, -1.0f);
	float HeaderX = x + (Width - HeaderWidth) / 2.0f; // Center the header
	
	TextRender()->Text(HeaderX, y + 10.0f, FontSize + 4.0f, aHeader);
	
	// Records
	for(int i = 0; i < m_NumRecords; i++)
	{
		const SMapTimeRecord &Record = m_aTopRecords[i];
		float RecordY = y + HeaderHeight + (i * LineHeight);
		
		// Rank color with more distinct colors
		if(i == 0)
			TextRender()->TextColor(1.0f, 0.8f, 0.0f, 1.0f); // Gold
		else if(i == 1)
			TextRender()->TextColor(0.9f, 0.9f, 0.9f, 1.0f); // Silver
		else if(i == 2)
			TextRender()->TextColor(0.9f, 0.6f, 0.2f, 1.0f); // Bronze
		else if(i < 5)
			TextRender()->TextColor(0.6f, 1.0f, 0.6f, 1.0f); // Light green for top 5
		else
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f); // White
		
		// Format time to show only 2 decimal places
		char aFormattedTime[32];
		FormatTime(aFormattedTime, sizeof(aFormattedTime), Record.m_aTime);
		
		// Format with padding for alignment: "1.  PlayerName        - 1:23.45"
		char aLine[256];
		str_format(aLine, sizeof(aLine), "%2d. %-20s - %s", i + 1, Record.m_aPlayerName, aFormattedTime);
		
		TextRender()->Text(x + 20.0f, RecordY, FontSize, aLine);
	}
	
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f); // Reset color
}
