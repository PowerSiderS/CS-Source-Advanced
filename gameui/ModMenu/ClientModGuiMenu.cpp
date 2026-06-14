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
        m_pPlayerCountPos = new CLabeledCommandComboBox( this, "PlayerCountPosComboBox" );
        m_pPlayerCountPos->AddItem( "#GameUI_GUI_PlayerCount_Bottom", "hud_playercount_pos 0" );
        m_pPlayerCountPos->AddItem( "#GameUI_GUI_PlayerCount_Top", "hud_playercount_pos 1" );

        m_pRadarSquare = new CCvarToggleCheckButton( this, "RadarSquareCheckBox", "#GameUI_RadarSquare", "cl_radar_square_with_scoreboard" );
        m_pRadarAlpha = new CCvarSlider( this, "RadarAlphaSlider", "#GameUI_RadarAlpha", 0.0f, 255.0f, "cl_radaralpha" );
        m_pRadarRotate = new CCvarToggleCheckButton( this, "RadarRotateCheckBox", "#GameUI_RadarRotate", "cl_radar_rotate" );
        m_pRadarScale = new CCvarSlider( this, "RadarScaleSlider", "#GameUI_RadarScale", 0.8f, 1.3f, "cl_radar_scale" );
        m_pXhairRainbow = new CCvarToggleCheckButton( this, "XhairRainbowCheckBox", "#GameUI_Crosshair_Rainbow", "xhair_rainbow" );
        m_pXhairSniper = new CCvarToggleCheckButton( this, "XhairSniperCheckBox", "#GameUI_Crosshair_Sniper", "xhair_sniper" );
        m_pEnableColorHud = new CCvarToggleCheckButton( this, "HudColorCheckBox", "#GameUI_Hud_Color", "hud_colored" );
        m_pDisplayRadarName = new CCvarToggleCheckButton( this, "RadarDisplayNameCheckBox", "#GameUI_Radar_Name_Display", "hud_radar_display_name" );
        m_pDisplayRadarHealth = new CCvarToggleCheckButton( this, "RadarDisplayHealthCheckBox", "#GameUI_Radar_Health_Display", "hud_radar_display_healthbar" );
        m_pDisplayRadarLine = new CCvarToggleCheckButton( this, "RadarDisplayLineCheckBox", "#GameUI_Radar_Line_Display", "hud_radar_display_line" );
        m_pDisplayRoundtimerC4 = new CCvarToggleCheckButton( this, "RoundtimerDisplayC4CheckBox", "#GameUI_Roundtimer_Display_C4", "hud_roundtimer_display_c4" );

        m_pPlayerCountPos->AddActionSignalTarget( this );
        m_pRadarSquare->AddActionSignalTarget( this );
        m_pRadarAlpha->AddActionSignalTarget( this );
        m_pRadarRotate->AddActionSignalTarget( this );
        m_pRadarScale->AddActionSignalTarget( this );
        m_pXhairRainbow->AddActionSignalTarget( this );
        m_pXhairSniper->AddActionSignalTarget( this );
        m_pEnableColorHud->AddActionSignalTarget( this );
        m_pDisplayRadarName->AddActionSignalTarget( this );
        m_pDisplayRadarHealth->AddActionSignalTarget( this );
        m_pDisplayRadarLine->AddActionSignalTarget( this );
        m_pDisplayRoundtimerC4->AddActionSignalTarget( this );

        LoadControlSettings( "Resource/OptionGuiMenu.res" );
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
        m_pPlayerCountPos->SetInitialItem( ConVarRef( "hud_playercount_pos" ).GetInt() );
        
        m_pRadarSquare->Reset();
        m_pRadarAlpha->Reset();
        m_pRadarRotate->Reset();
        m_pRadarScale->Reset();
        m_pXhairRainbow->Reset();
        m_pXhairSniper->Reset();
        m_pEnableColorHud->Reset();
        m_pDisplayRadarName->Reset();
        m_pDisplayRadarHealth->Reset();
        m_pDisplayRadarLine->Reset();
        m_pDisplayRoundtimerC4->Reset();
}

void ClientModGuiMenu::OnApplyChanges()
{
        m_pPlayerCountPos->ApplyChanges();
        
        m_pRadarSquare->ApplyChanges();
        m_pRadarAlpha->ApplyChanges();
        m_pRadarRotate->ApplyChanges();
        m_pRadarScale->ApplyChanges();
        m_pXhairRainbow->ApplyChanges();
        m_pXhairSniper->ApplyChanges();
        m_pEnableColorHud->ApplyChanges();
        m_pDisplayRadarName->ApplyChanges();
        m_pDisplayRadarHealth->ApplyChanges();
        m_pDisplayRadarLine->ApplyChanges();
        m_pDisplayRoundtimerC4->ApplyChanges();
}
