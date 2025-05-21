// IMyPanel.h
#include "vgui_controls/Panel.h"
#include "GameUI/IGameUI.h"
class IMyPanel
{
public:
	virtual void		Create(vgui::VPANEL parent) = 0;
	virtual void		Destroy(void) = 0;
};

extern IMyPanel* insolencemenu;
