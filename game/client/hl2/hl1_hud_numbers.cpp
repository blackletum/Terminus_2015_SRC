//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "hl1_hud_numbers.h"


// This is a bad way to implement HL1 style sprite fonts, but it will work for now

CHL1HudNumbers::CHL1HudNumbers( vgui::Panel *parent, const char *name ) : BaseClass( parent, name )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}


void CHL1HudNumbers::VidInit( void )
{
	for ( int i = 0; i < 10; i++ )
	{
		char szNumString[ 10 ];

		sprintf( szNumString, "number_%d", i );
		icon_digits[ i ] = gHUD.GetIcon( szNumString );
	}
	for (int i = 0; i < 10; i++)
	{
		char szNumStringSmall[16];

		sprintf(szNumStringSmall, "number_%d_small", i);
		icon_sdigits[i] = gHUD.GetIcon(szNumStringSmall);
	}
	icon_dull1 = gHUD.GetIcon("empty_number_1");
	icon_dull2 = gHUD.GetIcon("empty_number_2");
	icon_dull1s = gHUD.GetIcon("empty_number_1_s");
	icon_dull2s = gHUD.GetIcon("empty_number_2_s");
}


int CHL1HudNumbers::GetNumberFontHeight( void )
{
	if ( icon_digits[ 0 ] )
	{
		return icon_digits[ 0 ]->Height();
	}
	else
	{
		return 0;
	}
}
int CHL1HudNumbers::GetSmallNumberFontHeight(void)
{
	if (icon_sdigits[0])
	{
		return icon_sdigits[0]->Height();
	}
	else
	{
		return 0;
	}
}


int CHL1HudNumbers::GetNumberFontWidth( void )
{
	if (icon_digits[0])
	{
		return icon_digits[0]->Width();
	}
	else
	{
		return 0;
	}
}
int CHL1HudNumbers::GetSmallNumberFontWidth(void)
{
	if (icon_sdigits[0])
	{
		return icon_sdigits[0]->Width();
	}
	else
	{
		return 0;
	}
}


int CHL1HudNumbers::DrawHudNumber( int x, int y, int iNumber, Color &clrDraw, bool SmallNum )
{
	int iWidth = GetNumberFontWidth();
	int iWidthS = GetSmallNumberFontWidth();
	int k;
	//related to the color for the dull numbers
	Color clrDraws;
	int r, g, b, nUnused;
	(gHUD.m_clrNormal).GetColor(r, g, b, nUnused);
	clrDraws.SetColor(r, g, b, 255);
	//

	if (!SmallNum)
	{
		if (iNumber > 0)
		{
			icon_dull1->DrawSelf(x, y, clrDraw);
			// SPR_Draw 100's
			if (iNumber >= 100)
			{
				k = iNumber / 100;
				icon_digits[k]->DrawSelf(x, y, clrDraw);
				icon_dull1->DrawSelf(x, y, clrDraws);
				x += iWidth;
			}
			else
			{
				x += iWidth;
			}

			icon_dull2->DrawSelf(x, y, clrDraws);
			// SPR_Draw 10's
			if (iNumber >= 10)
			{
				k = (iNumber % 100) / 10;
				icon_digits[k]->DrawSelf(x, y, clrDraw);
				icon_dull2->DrawSelf(x, y, clrDraws);
				x += iWidth;
			}
			else
			{
				x += iWidth;
			}
			icon_dull1->DrawSelf(x, y, clrDraws);
			// SPR_Draw ones
			k = iNumber % 10;
			icon_digits[k]->DrawSelf(x, y, clrDraw);
			x += iWidth;
		}
		else
		{
			// SPR_Draw 100's
			icon_dull1->DrawSelf(x, y, clrDraws);
			x += iWidth;
			// SPR_Draw 10's
			icon_dull2->DrawSelf(x, y, clrDraws);
			x += iWidth;

			// SPR_Draw ones
			k = 0;
			icon_digits[k]->DrawSelf(x, y, clrDraw);
			icon_dull1->DrawSelf(x, y, clrDraws);
			x += iWidth;
		}
	}
	else //small numbers
	{
		if (iNumber > 0)
		{
			icon_dull1s->DrawSelf(x, y, clrDraws);
			// SPR_Draw 100's
			if (iNumber >= 100)
			{
				k = iNumber / 100;
				icon_sdigits[k]->DrawSelf(x, y, clrDraw);
				icon_dull1s->DrawSelf(x, y, clrDraws);
				x += iWidthS;
			}
			else
			{
				x += iWidthS;
			}

			icon_dull2s->DrawSelf(x, y, clrDraws);
			// SPR_Draw 10's
			if (iNumber >= 10)
			{
				k = (iNumber % 100) / 10;
				icon_sdigits[k]->DrawSelf(x, y, clrDraw);
				icon_dull2s->DrawSelf(x, y, clrDraws);
				x += iWidthS;
			}
			else
			{
				x += iWidthS;
			}
			icon_dull1s->DrawSelf(x, y, clrDraws);
			// SPR_Draw ones
			k = iNumber % 10;
			icon_sdigits[k]->DrawSelf(x, y, clrDraw);
			x += iWidthS;
		}
		else
		{
			// SPR_Draw 100's
			x += iWidthS;

			// SPR_Draw 10's
			x += iWidthS;

			// SPR_Draw ones
			k = 0;
			icon_sdigits[k]->DrawSelf(x, y, clrDraw);
			icon_dull1s->DrawSelf(x, y, clrDraws);
			x += iWidthS;
		}
	}

	return x;
}
