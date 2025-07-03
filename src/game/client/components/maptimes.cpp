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
	
	// Parse the first 5 records
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
