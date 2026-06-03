//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Centralized HUD color system for CS:S Android
// Author: PowerSiderS
//
//=============================================================================//

#ifndef CS_HUD_COLOR_H
#define CS_HUD_COLOR_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include <Color.h>

// hud_colored   - 1 = coloring on, 0 = no coloring
extern ConVar hud_color;
extern ConVar hud_colored;

// Get rainbow color that cycles over time (utility, still usable for effects)
inline Color GetRainbowColor( float timeOffset = 0.0f )
{
        float time = gpGlobals->curtime + timeOffset;
        float frequency = 2.0f;

        int r = (int)(sin(frequency * time + 0) * 127 + 128);
        int g = (int)(sin(frequency * time + 2) * 127 + 128);
        int b = (int)(sin(frequency * time + 4) * 127 + 128);

        return Color( r, g, b, 255 );
}

inline Color GetHudColor( int alpha = 255 )
{
        if ( !hud_colored.GetBool() )
                return Color( 255, 170, 0, alpha );

        int r = 255, g = 50, b = 50;
        const char *pszColor = hud_color.GetString();
        if ( pszColor && *pszColor )
        {
                sscanf( pszColor, "%d %d %d", &r, &g, &b );
                r = clamp( r, 0, 255 );
                g = clamp( g, 0, 255 );
                b = clamp( b, 0, 255 );
        }

        return Color( r, g, b, alpha );
}

// Get HUD color with custom alpha (for transparency effects)
inline Color GetHudColorWithAlpha( int alpha )
{
        Color clr = GetHudColor( 255 );
        clr[3] = alpha;
        return clr;
}

inline Color GetHudColorSecondary( int alpha = 255 )
{
        if ( !hud_colored.GetBool() )
                return Color( 178, 119, 0, alpha );

        int r = 255, g = 50, b = 50;
        const char *pszColor = hud_color.GetString();
        if ( pszColor && *pszColor )
        {
                sscanf( pszColor, "%d %d %d", &r, &g, &b );
                r = clamp( r, 0, 255 );
                g = clamp( g, 0, 255 );
                b = clamp( b, 0, 255 );
        }

        // Darken by ~30% for the secondary shade
        r = (r * 7) / 10;
        g = (g * 7) / 10;
        b = (b * 7) / 10;

        return Color( r, g, b, alpha );
}

#endif // CS_HUD_COLOR_H
