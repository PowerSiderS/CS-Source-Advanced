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

// Get rainbow color that cycles over time.
inline Color GetRainbowColor( float timeOffset = 0.0f )
{
        static int   s_cachedFrame = -1;
        static float s_r = 128.0f, s_g = 128.0f, s_b = 128.0f;

        if ( timeOffset == 0.0f )
        {
                int curFrame = gpGlobals->framecount;
                if ( curFrame != s_cachedFrame )
                {
                        float t = gpGlobals->curtime * 2.0f;
                        s_r = sinf( t )       * 127.0f + 128.0f;
                        s_g = sinf( t + 2.0f ) * 127.0f + 128.0f;
                        s_b = sinf( t + 4.0f ) * 127.0f + 128.0f;
                        s_cachedFrame = curFrame;
                }
                return Color( (int)s_r, (int)s_g, (int)s_b, 255 );
        }

        // Non-zero offset: compute directly (rare / unused in practice)
        float t = ( gpGlobals->curtime + timeOffset ) * 2.0f;
        return Color(
                (int)( sinf( t )        * 127.0f + 128.0f ),
                (int)( sinf( t + 2.0f ) * 127.0f + 128.0f ),
                (int)( sinf( t + 4.0f ) * 127.0f + 128.0f ),
                255 );
}

inline void GetHudColorRGB( int &r, int &g, int &b )
{
        static char s_cachedStr[32] = "";
        static int  s_r = 255, s_g = 50, s_b = 50;

        const char *pszColor = hud_color.GetString();
        if ( !pszColor || !*pszColor )
        {
                r = 255; g = 50; b = 50;
                return;
        }

        if ( Q_strcmp( pszColor, s_cachedStr ) != 0 )
        {
                Q_strncpy( s_cachedStr, pszColor, sizeof( s_cachedStr ) );
                s_r = 255; s_g = 50; s_b = 50;
                sscanf( pszColor, "%d %d %d", &s_r, &s_g, &s_b );
                s_r = clamp( s_r, 0, 255 );
                s_g = clamp( s_g, 0, 255 );
                s_b = clamp( s_b, 0, 255 );
        }

        r = s_r; g = s_g; b = s_b;
}

inline Color GetHudColor( int alpha = 255 )
{
        if ( !hud_colored.GetBool() )
                return Color( 255, 170, 0, alpha );

        int r, g, b;
        GetHudColorRGB( r, g, b );
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

        int r, g, b;
        GetHudColorRGB( r, g, b );

        // Darken by ~30% for the secondary shade
        return Color( (r * 7) / 10, (g * 7) / 10, (b * 7) / 10, alpha );
}

#endif // CS_HUD_COLOR_H
