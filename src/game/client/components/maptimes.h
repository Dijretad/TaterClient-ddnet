/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MAPTIMES_H
#define GAME_CLIENT_COMPONENTS_MAPTIMES_H

#include <engine/shared/http.h>
#include <engine/textrender.h>
#include <engine/shared/protocol.h>
#include <engine/console.h>
#include <game/client/component.h>
#include <memory>

class IConsole;

struct SMapTimeRecord
{
	char m_aPlayerName[MAX_NAME_LENGTH];
	char m_aTime[32]; // Format: "00:20:02.440000"
	int m_Rank;
	
	SMapTimeRecord()
	{
		m_aPlayerName[0] = '\0';
		m_aTime[0] = '\0';
		m_Rank = 0;
	}
};

class CMapTimes : public CComponent
{
	enum
	{
		MAX_TOP_RECORDS = 10,
		STATE_IDLE,
		STATE_LOADING,
		STATE_DONE,
		STATE_ERROR
	};

	SMapTimeRecord m_aTopRecords[MAX_TOP_RECORDS];
	int m_NumRecords;
	int m_State;
	std::shared_ptr<CHttpRequest> m_pRequest;
	char m_aCurrentMap[64];
	int64_t m_LastRequestTime;
	int64_t m_LastUpdateTime;
	bool m_PendingChatDisplay; // Flag to show results in chat when they arrive
	
	// Text containers for rendering
	STextContainerIndex m_aPlayerNameContainers[MAX_TOP_RECORDS];
	STextContainerIndex m_aTimeContainers[MAX_TOP_RECORDS];
	STextContainerIndex m_HeaderContainer;
	bool m_ContainersValid;
	
	void RequestMapTimes(const char *pMapName);
	void ParseResponse(const char *pJson);
	void UpdateTextContainers();
	void ResetTextContainers();

public:
	CMapTimes();
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnReset() override;
	virtual void OnRender() override;
	virtual void OnMapLoad() override;
	virtual void OnInit() override;
	
	void Update();
	void RenderMapTimes(float x, float y, float w, float h);
	void RenderMapTimesTab(float x, float y); // Nouveau rendu pour le menu Tab
	void RenderMapTimesFullscreen(float x, float y); // Nouveau rendu fullscreen
	bool HasValidData() const { return m_State == STATE_DONE && m_NumRecords > 0; }
	
	// Accessors for leaderboard display
	int GetNumRecords() const { return m_NumRecords; }
	const SMapTimeRecord *GetRecord(int Index) const { return (Index >= 0 && Index < m_NumRecords) ? &m_aTopRecords[Index] : nullptr; }
	const char *GetCurrentMap() const { return m_aCurrentMap; }
	void FormatTime(char *pBuffer, int BufferSize, const char *pTimeString);
	
	// Command functions
	void ShowTop10InChat();
	static void ConShowTop10(IConsole::IResult *pResult, void *pUser);
	static void ConKeyMapTimes(IConsole::IResult *pResult, void *pUserData);
};

#endif
