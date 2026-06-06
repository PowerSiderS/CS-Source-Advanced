#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h>
#endif

#include "ClientModCrosshairMenu.h"
#include <stdio.h>

#include <vgui_controls/Button.h>
#include "tier1/KeyValues.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui_controls/ComboBox.h>
#include "vgui_controls/QueryBox.h"

#include "CvarTextEntry.h"
#include "CvarToggleCheckButton.h"
#include "cvarslider.h"
#include "LabeledCommandComboBox.h"
#include "EngineInterface.h"
#include "tier1/convar.h"

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

#include <tier0/memdbgon.h>

using namespace vgui;

struct ColorPreset_t
{
        const char *name;
        int r, g, b;
};

static ColorPreset_t s_crosshairColors[] = 
{
        { "Green",              50,             250,    50 },
        { "Red",                250,    50,             50 },
        { "Blue",               50,             50,             250 },
        { "Yellow",             250,    250,    50 },
        { "Cyan",               50,             250,    250 },
        { "Ltblue",             50,             200,    250 },
        { "Custom",             0,              0,              0 }
};

class CrosshairImagePanelMod : public ImagePanel
{
        DECLARE_CLASS_SIMPLE( CrosshairImagePanelMod, ImagePanel );

public:
        CrosshairImagePanelMod( Panel *parent, const char *name, ClientModCrosshairMenu* pOptionsPanel );
        virtual void ResetData();
        virtual void ApplyChanges();

protected:
        MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", data );
        MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
        MESSAGE_FUNC( OnCheckButtonChecked, "CheckButtonChecked" );

        virtual void Paint();
        void DrawCrosshairRect( int r, int g, int b, int a, int x0, int y0, int x1, int y1, bool bAdditive );
        void UpdateCrosshair();

private:
        ClientModCrosshairMenu  *m_pOptionsPanel;
        CLabeledCommandComboBox *m_pCrosshairStyle;
        CCvarSlider                             *m_pCrosshairAlpha;
        CCvarToggleCheckButton  *m_pCrosshairUseAlpha;
        CCvarSlider                             *m_pCrosshairGap;
        CCvarToggleCheckButton  *m_pCrosshairGapUseWeaponValue;
        CCvarSlider                             *m_pCrosshairSize;
        CCvarSlider                             *m_pCrosshairThickness;
        CCvarToggleCheckButton  *m_pCrosshairDot;
        CCvarSlider                             *m_pCrosshairColorR;
        CCvarSlider                             *m_pCrosshairColorG;
        CCvarSlider                             *m_pCrosshairColorB;
        CCvarToggleCheckButton  *m_pCrosshairDrawOutline;
        CCvarSlider                             *m_pCrosshairOutlineThickness;
        CCvarToggleCheckButton  *m_pCrosshairT;
        CLabeledCommandComboBox *m_pCrosshairColor;
        int m_iCrosshairTextureID;
};

CrosshairImagePanelMod::CrosshairImagePanelMod( Panel *parent, const char *name, ClientModCrosshairMenu* pOptionsPanel ) : ImagePanel( parent, name )
{
        m_pOptionsPanel = pOptionsPanel;
        
        m_pCrosshairStyle = new CLabeledCommandComboBox( m_pOptionsPanel, "CrosshairStyle" );
        m_pCrosshairAlpha = new CCvarSlider( m_pOptionsPanel, "CrosshairAlpha", "#GameUI_Crosshair_Alpha", 0.0f, 255.0f, "xhair_color_a" );
        m_pCrosshairUseAlpha = new CCvarToggleCheckButton( m_pOptionsPanel, "CrosshairUseAlpha", "#GameUI_Crosshair_UseAlpha", "xhair_usealpha" );
        m_pCrosshairGap = new CCvarSlider( m_pOptionsPanel, "CrosshairGap", "#GameUI_Crosshair_Gap", -2.0f, 7.0f, "xhair_gap" );
        m_pCrosshairGapUseWeaponValue = new CCvarToggleCheckButton( m_pOptionsPanel, "CrosshairGapUseWeaponValue", "#GameUI_Crosshair_Gap_UseWeaponValue", "xhair_gap_useweapon" );
        m_pCrosshairSize = new CCvarSlider( m_pOptionsPanel, "CrosshairSize", "#GameUI_Crosshair_Size", 0.0f, 10.0f, "xhair_size" );
        m_pCrosshairThickness = new CCvarSlider( m_pOptionsPanel, "CrosshairThickness", "#GameUI_Crosshair_Thickness", 0.1f, 6.0f, "xhair_thickness" );
        m_pCrosshairDot = new CCvarToggleCheckButton( m_pOptionsPanel, "CrosshairDot", "#GameUI_Crosshair_Dot", "xhair_dot" );
        m_pCrosshairColorR = new CCvarSlider( m_pOptionsPanel, "CrosshairColorR", "#GameUI_Crosshair_Color_R", 0.0f, 255.0f, "xhair_color_r" );
        m_pCrosshairColorG = new CCvarSlider( m_pOptionsPanel, "CrosshairColorG", "#GameUI_Crosshair_Color_G", 0.0f, 255.0f, "xhair_color_g" );
        m_pCrosshairColorB = new CCvarSlider( m_pOptionsPanel, "CrosshairColorB", "#GameUI_Crosshair_Color_B", 0.0f, 255.0f, "xhair_color_b" );
        m_pCrosshairDrawOutline = new CCvarToggleCheckButton( m_pOptionsPanel, "CrosshairDrawOutline", "#GameUI_Crosshair_DrawOutline", "xhair_outline" );
        m_pCrosshairOutlineThickness = new CCvarSlider( m_pOptionsPanel, "CrosshairOutlineThickness", "#GameUI_Crosshair_OutlineThickness", 0.0f, 3.0f, "xhair_outline_adjust" );
        m_pCrosshairT = new CCvarToggleCheckButton( m_pOptionsPanel, "CrosshairT", "#GameUI_Crosshair_T", "xhair_t_style" );
        m_pCrosshairColor = new CLabeledCommandComboBox( m_pOptionsPanel, "CrosshairColor" );

        m_pCrosshairStyle->AddItem( "#GameUI_Crosshair_Style_0", "xhair_style 0" );
        m_pCrosshairStyle->AddItem( "#GameUI_Crosshair_Style_1", "xhair_style 1" );
        m_pCrosshairStyle->AddItem( "#GameUI_Crosshair_Style_2", "xhair_style 2" );
        m_pCrosshairStyle->AddItem( "#GameUI_Crosshair_Style_3", "xhair_style 3" );
        m_pCrosshairStyle->AddItem( "#GameUI_Crosshair_Style_4", "xhair_style 4" );
        m_pCrosshairStyle->AddItem( "#GameUI_Crosshair_Style_5", "xhair_style 5" );

        for ( int i = 0; i < ARRAYSIZE( s_crosshairColors ); i++ )
        {
                char command[128];
                if ( i < ARRAYSIZE( s_crosshairColors ) - 1 )
                        Q_snprintf( command, sizeof( command ), "xhair_color_r %d; xhair_color_g %d; xhair_color_b %d", 
                                s_crosshairColors[i].r, s_crosshairColors[i].g, s_crosshairColors[i].b );
                else
                        command[0] = '\0';
                m_pCrosshairColor->AddItem( s_crosshairColors[i].name, command );
        }

        m_pCrosshairStyle->AddActionSignalTarget( this );
        m_pCrosshairAlpha->AddActionSignalTarget( this );
        m_pCrosshairUseAlpha->AddActionSignalTarget( this );
        m_pCrosshairGap->AddActionSignalTarget( this );
        m_pCrosshairGapUseWeaponValue->AddActionSignalTarget( this );
        m_pCrosshairSize->AddActionSignalTarget( this );
        m_pCrosshairThickness->AddActionSignalTarget( this );
        m_pCrosshairDot->AddActionSignalTarget( this );
        m_pCrosshairColorR->AddActionSignalTarget( this );
        m_pCrosshairColorG->AddActionSignalTarget( this );
        m_pCrosshairColorB->AddActionSignalTarget( this );
        m_pCrosshairDrawOutline->AddActionSignalTarget( this );
        m_pCrosshairOutlineThickness->AddActionSignalTarget( this );
        m_pCrosshairT->AddActionSignalTarget( this );
        m_pCrosshairColor->AddActionSignalTarget( this );

        m_iCrosshairTextureID = vgui::surface()->CreateNewTextureID();
        vgui::surface()->DrawSetTextureFile( m_iCrosshairTextureID, "vgui/white_additive", true, false );

        SetImage( "crosshair/de_dust2" );
        ResetData();
}

void CrosshairImagePanelMod::DrawCrosshairRect( int r, int g, int b, int a, int x0, int y0, int x1, int y1, bool bAdditive )
{
        if ( m_pCrosshairDrawOutline->IsSelected() )
        {
                float flThick = m_pCrosshairOutlineThickness->GetSliderValue();
                vgui::surface()->DrawSetColor( 0, 0, 0, a );
                vgui::surface()->DrawFilledRect( x0-flThick, y0-flThick, x1+flThick, y1+flThick );
        }

        vgui::surface()->DrawSetColor( r, g, b, a );

        if ( bAdditive )
        {
                vgui::surface()->DrawTexturedRect( x0, y0, x1, y1 );
        }
        else
        {
                vgui::surface()->DrawFilledRect( x0, y0, x1, y1 );
        }
}

void CrosshairImagePanelMod::Paint()
{
        int screenWide, screenTall;
        surface()->GetScreenSize( screenWide, screenTall );

        BaseClass::Paint();

        int wide, tall;
        GetSize( wide, tall );

        bool bAdditive = !m_pCrosshairUseAlpha->IsSelected();

        int a = 200;
        if ( !bAdditive )
                a = m_pCrosshairAlpha->GetSliderValue();

        int r, g, b;
        int colorIdx = m_pCrosshairColor->GetActiveItem();
        if ( colorIdx >= 0 && colorIdx < ARRAYSIZE( s_crosshairColors ) - 1 )
        {
                r = s_crosshairColors[colorIdx].r;
                g = s_crosshairColors[colorIdx].g;
                b = s_crosshairColors[colorIdx].b;
        }
        else
        {
                r = m_pCrosshairColorR->GetSliderValue();
                g = m_pCrosshairColorG->GetSliderValue();
                b = m_pCrosshairColorB->GetSliderValue();
        }

        vgui::surface()->DrawSetColor( r, g, b, a );

        if ( bAdditive )
        {
                vgui::surface()->DrawSetTexture( m_iCrosshairTextureID );
        }

        int centerX = wide / 2;
        int centerY = tall / 2;

        int iBarSize = RoundFloatToInt( m_pCrosshairSize->GetSliderValue() * screenTall / 480.0f );
        int iBarThickness = max( 1, RoundFloatToInt( m_pCrosshairThickness->GetSliderValue() * (float)screenTall / 480.0f ) );
        int iBarGap = m_pCrosshairGap->GetSliderValue();

        int iInnerLeft = centerX - iBarGap - iBarThickness / 2;
        int iInnerRight = iInnerLeft + 2 * iBarGap + iBarThickness;
        int iOuterLeft = iInnerLeft - iBarSize;
        int iOuterRight = iInnerRight + iBarSize;
        int y0 = centerY - iBarThickness / 2;
        int y1 = y0 + iBarThickness;
        DrawCrosshairRect( r, g, b, a, iOuterLeft, y0, iInnerLeft, y1, bAdditive );
        DrawCrosshairRect( r, g, b, a, iInnerRight, y0, iOuterRight, y1, bAdditive );

        int iInnerTop = centerY - iBarGap - iBarThickness / 2;
        int iInnerBottom = iInnerTop + 2 * iBarGap + iBarThickness;
        int iOuterTop = iInnerTop - iBarSize;
        int iOuterBottom = iInnerBottom + iBarSize;
        int x0 = centerX - iBarThickness / 2;
        int x1 = x0 + iBarThickness;
        if ( !m_pCrosshairT->IsSelected() )
                DrawCrosshairRect( r, g, b, a, x0, iOuterTop, x1, iInnerTop, bAdditive );
        DrawCrosshairRect( r, g, b, a, x0, iInnerBottom, x1, iOuterBottom, bAdditive );

        if ( m_pCrosshairDot->IsSelected() )
        {
                x0 = centerX - iBarThickness / 2;
                x1 = x0 + iBarThickness;
                y0 = centerY - iBarThickness / 2;
                y1 = y0 + iBarThickness;
                DrawCrosshairRect( r, g, b, a, x0, y0, x1, y1, bAdditive );
        }
}

void CrosshairImagePanelMod::UpdateCrosshair()
{
}

void CrosshairImagePanelMod::OnSliderMoved( KeyValues *data )
{
        m_pOptionsPanel->OnControlModified();
        UpdateCrosshair();
}

void CrosshairImagePanelMod::OnTextChanged( vgui::Panel *panel )
{
        bool bCustomColor = ( m_pCrosshairColor->GetActiveItem() == ARRAYSIZE( s_crosshairColors ) - 1 );
        m_pCrosshairColorR->SetEnabled( bCustomColor );
        m_pCrosshairColorG->SetEnabled( bCustomColor );
        m_pCrosshairColorB->SetEnabled( bCustomColor );
        m_pOptionsPanel->OnControlModified();
        UpdateCrosshair();
}

void CrosshairImagePanelMod::OnCheckButtonChecked()
{
        m_pCrosshairAlpha->SetEnabled( m_pCrosshairUseAlpha->IsSelected() );
        m_pCrosshairOutlineThickness->SetEnabled( m_pCrosshairDrawOutline->IsSelected() );
        m_pOptionsPanel->OnControlModified();
        UpdateCrosshair();
}

void CrosshairImagePanelMod::ResetData()
{
        ConVarRef xhair_style( "xhair_style" );
        if ( xhair_style.IsValid() )
                m_pCrosshairStyle->SetInitialItem( xhair_style.GetInt() );

        m_pCrosshairAlpha->Reset();
        m_pCrosshairUseAlpha->Reset();
        m_pCrosshairGap->Reset();
        m_pCrosshairGapUseWeaponValue->Reset();
        m_pCrosshairSize->Reset();
        m_pCrosshairThickness->Reset();
        m_pCrosshairDot->Reset();
        m_pCrosshairColorR->Reset();
        m_pCrosshairColorG->Reset();
        m_pCrosshairColorB->Reset();
        m_pCrosshairDrawOutline->Reset();
        m_pCrosshairOutlineThickness->Reset();
        m_pCrosshairT->Reset();

        m_pCrosshairColor->SetInitialItem( ARRAYSIZE( s_crosshairColors ) - 1 );

        UpdateCrosshair();
}

void CrosshairImagePanelMod::ApplyChanges()
{
        m_pCrosshairStyle->ApplyChanges();
        m_pCrosshairAlpha->ApplyChanges();
        m_pCrosshairUseAlpha->ApplyChanges();
        m_pCrosshairGap->ApplyChanges();
        m_pCrosshairGapUseWeaponValue->ApplyChanges();
        m_pCrosshairSize->ApplyChanges();
        m_pCrosshairThickness->ApplyChanges();
        m_pCrosshairDot->ApplyChanges();
        m_pCrosshairColorR->ApplyChanges();
        m_pCrosshairColorG->ApplyChanges();
        m_pCrosshairColorB->ApplyChanges();
        m_pCrosshairDrawOutline->ApplyChanges();
        m_pCrosshairOutlineThickness->ApplyChanges();
        m_pCrosshairT->ApplyChanges();
        m_pCrosshairColor->ApplyChanges();
}

ClientModCrosshairMenu::ClientModCrosshairMenu( vgui::Panel *parent ): vgui::PropertyPage( parent, "ClientModCrosshairMenu" )
{
        m_pCrosshairImage = new CrosshairImagePanelMod( this, "CrosshairImage", this );

        LoadControlSettings( "Resource/OptionCrosshairMenu.res" );
}

ClientModCrosshairMenu::~ClientModCrosshairMenu()
{
}

void ClientModCrosshairMenu::OnControlModified()
{
        PostMessage( GetParent(), new KeyValues( "ApplyButtonEnable" ) );
        InvalidateLayout();
}

void ClientModCrosshairMenu::OnResetData()
{
        if ( m_pCrosshairImage )
                m_pCrosshairImage->ResetData();
}

void ClientModCrosshairMenu::OnApplyChanges()
{
        if ( m_pCrosshairImage )
                m_pCrosshairImage->ApplyChanges();
}
