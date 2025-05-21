//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HL1_HUD_NUMBERS_H
#define HL1_HUD_NUMBERS_H
#ifdef _WIN32
#pragma once
#endif


#include <vgui_controls/Panel.h>


class CHL1HudNumbers : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHL1HudNumbers, vgui::Panel );

public:
	CHL1HudNumbers( vgui::Panel *parent, const char *name );
	void	VidInit( void );

protected:
	int		DrawHudNumber( int x, int y, int iNumber, Color &clrDraw, bool SmallNum );
	int		GetNumberFontHeight( void );
	int		GetSmallNumberFontHeight(void);
	int		GetNumberFontWidth( void );
	int		GetSmallNumberFontWidth(void);

private:
	CHudTexture *icon_digits[10];
	CHudTexture *icon_sdigits[10];
	CHudTexture *icon_dull1;
	CHudTexture *icon_dull2;
	CHudTexture *icon_dull1s;
	CHudTexture *icon_dull2s;
};


#endif // HL1_HUD_NUMBERS_H
