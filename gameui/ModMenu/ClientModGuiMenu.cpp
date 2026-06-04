#include "ClientModGuiMenu.h"
#include <stdio.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ComboBox.h>
#include "tier1/KeyValues.h"

#include "CvarToggleCheckButton.h"
#include "cvarslider.h"
#include "LabeledCommandComboBox.h"
#include "tier1/convar.h"

#include <tier0/memdbgon.h>

using namespace vgui;

ClientModGuiMenu::ClientModGuiMenu( vgui::Panel *parent ) : vgui::PropertyPage( parent, "ClientModGuiMenu" )
{
        m_pHudStyle = new CLabeledCommandComboBox( this, "HudHealthArmorStyleComboBox" );
        m_pHudStyle->AddItem( "Default", "hud_healtharmor_style 0" );
        m_pHudStyle->AddItem( "Simple", "hud_healtharmor_style 1" );

        m_pPlayerCountPos = new CLabeledCommandComboBox( this, "PlayerCountPosComboBox" );
        m_pPlayerCountPos->AddItem( "Top", "hud_playercount_pos 0" );
        m_pPlayerCountPos->AddItem( "Bottom", "hud_playercount_pos 1" );

        m_pHudColor = new CLabeledCommandComboBox( this, "HudColorComboBox" );
        m_pHudColor->AddItem( "Red", "cl_hud_color 0" );
        m_pHudColor->AddItem( "Blue", "cl_hud_color 1" );
        m_pHudColor->AddItem( "Cyan", "cl_hud_color 2" );
        m_pHudColor->AddItem( "Lime", "cl_hud_color 3" );
        m_pHudColor->AddItem( "Rainbow", "cl_hud_color 4" );

        m_pRadarSquare = new CCvarToggleCheckButton( this, "RadarSquareCheckBox", "#GameUI_RadarSquare", "cl_radar_square_with_scoreboard" );
        m_pRadarAlpha = new CCvarSlider( this, "RadarAlphaSlider", "#GameUI_RadarAlpha", 0.0f, 255.0f, "cl_radaralpha" );
        m_pRadarRotate = new CCvarToggleCheckButton( this, "RadarRotateCheckBox", "#GameUI_RadarRotate", "cl_radar_rotate" );
        m_pRadarScale = new CCvarSlider( this, "RadarScaleSlider", "#GameUI_RadarScale", 0.8f, 1.3f, "cl_radar_scale" );
        m_pXhairRainbow = new CCvarToggleCheckButton( this, "XhairRainbowCheckBox", "Crosshair is Rainbow", "xhair_rainbow" );

        m_pHudStyle->AddActionSignalTarget( this );
        m_pPlayerCountPos->AddActionSignalTarget( this );
        m_pHudColor->AddActionSignalTarget( this );
        m_pRadarSquare->AddActionSignalTarget( this );
        m_pRadarAlpha->AddActionSignalTarget( this );
        m_pRadarRotate->AddActionSignalTarget( this );
        m_pRadarScale->AddActionSignalTarget( this );
        m_pXhairRainbow->AddActionSignalTarget( this );

        LoadControlSettings( "Resource/ModGuiMenu.res" );
}

ClientModGuiMenu::~ClientModGuiMenu()
{
}

void ClientModGuiMenu::OnControlModified()
{
        PostMessage( GetParent(), new KeyValues( "ApplyButtonEnable" ) );
        InvalidateLayout();
}

void ClientModGuiMenu::OnTextChanged( vgui::Panel *panel )
{
        OnControlModified();
}

void ClientModGuiMenu::OnSliderMoved( KeyValues *data )
{
        OnControlModified();
}

void ClientModGuiMenu::OnCheckButtonChecked()
{
        OnControlModified();
}

void ClientModGuiMenu::OnResetData()
{
        m_pHudStyle->SetInitialItem( ConVarRef( "hud_healtharmor_style" ).GetInt() );
        m_pPlayerCountPos->SetInitialItem( ConVarRef( "hud_playercount_pos" ).GetInt() );
        m_pHudColor->SetInitialItem( ConVarRef( "cl_hud_color" ).GetInt() );
        
        m_pRadarSquare->Reset();
        m_pRadarAlpha->Reset();
        m_pRadarRotate->Reset();
        m_pRadarScale->Reset();
        m_pXhairRainbow->Reset();
}

void ClientModGuiMenu::OnApplyChanges()
{
        m_pHudStyle->ApplyChanges();
        m_pPlayerCountPos->ApplyChanges();
        m_pHudColor->ApplyChanges();
        
        m_pRadarSquare->ApplyChanges();
        m_pRadarAlpha->ApplyChanges();
        m_pRadarRotate->ApplyChanges();
        m_pRadarScale->ApplyChanges();
        m_pXhairRainbow->ApplyChanges();
}
