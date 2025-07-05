/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MAPTIMES_MENU_H
#define GAME_CLIENT_COMPONENTS_MAPTIMES_MENU_H

#include <game/client/component.h>

class CMapTimesMenu : public CComponent
{
	bool m_Active;
	bool m_WasActive;
	int64_t m_LastToggle;

public:
	CMapTimesMenu();
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnRender() override;
	virtual void OnRelease() override;
	virtual void OnInit() override;
	virtual bool OnCursorMove(float x, float y, IInput::ECursorType CursorType) override;
	virtual bool OnInput(const IInput::CEvent &Event) override;

	void Toggle();
	bool IsActive() const { return m_Active; }
	void SetActive(bool Active);
};

#endif
