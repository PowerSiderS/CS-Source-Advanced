#include "ClientModGuiMenu.h"
#include <stdio.h>
#include <string.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/TextEntry.h>
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
	
	m_pRadarAlpha         = new CCvarSlider( this, "RadarAlphaSlider", "#GameUI_RadarAlpha", 0.0f, 255.0f, "cl_radaralpha" );
	m_pRadarScale         = new CCvarSlider( this, "RadarScaleSlider", "#GameUI_RadarScale", 0.8f, 1.3f, "cl_radar_scale" );
	m_pRadarSize         = new CCvarSlider( this, "RadarScaleSize", "#GameUI_RadarSize", 0.5f, 2.5f, "cl_radar_panel_scale" );
	
	m_pRadarSquare        = new CCvarToggleCheckButton( this, "RadarSquareCheckBox",         "#GameUI_RadarSquare",            "cl_radar_square_with_scoreboard" );
	m_pRadarRotate        = new CCvarToggleCheckButton( this, "RadarRotateCheckBox",          "#GameUI_RadarRotate",            "cl_radar_rotate" );
	m_pXhairRainbow       = new CCvarToggleCheckButton( this, "XhairRainbowCheckBox",         "#GameUI_Crosshair_Rainbow",      "xhair_rainbow" );
	m_pXhairSniper        = new CCvarToggleCheckButton( this, "XhairSniperCheckBox",          "#GameUI_Crosshair_Sniper",       "xhair_sniper" );
	m_pEnableColorHud     = new CCvarToggleCheckButton( this, "HudColorCheckBox",             "#GameUI_Hud_Color",              "hud_colored" );
	m_pDisplayRadarName   = new CCvarToggleCheckButton( this, "RadarDisplayNameCheckBox",     "#GameUI_Radar_Name_Display",     "hud_radar_display_name" );
	m_pDisplayRadarHealth = new CCvarToggleCheckButton( this, "RadarDisplayHealthCheckBox",   "#GameUI_Radar_Health_Display",   "hud_radar_display_healthbar" );
	m_pDisplayRadarLine   = new CCvarToggleCheckButton( this, "RadarDisplayLineCheckBox",     "#GameUI_Radar_Line_Display",     "hud_radar_display_line" );
	m_pDisplayRoundtimerC4= new CCvarToggleCheckButton( this, "RoundtimerDisplayC4CheckBox", "#GameUI_Roundtimer_Display_C4",  "hud_roundtimer_display_c4" );
	#ifdef ANDROID
    m_pKeyboardMouseMode  = new CCvarToggleCheckButton( this, "KeyboardMouseModeCheckBox",   "#GameUI_KeyboardMouseMode",      "cl_keyboard_mouse_mode" );
    #endif

	// Hex color text entry (RRGGBB)
	m_pHudColorEntry = new TextEntry( this, "HudColorEntry" );
	m_pHudColorEntry->SetMaximumCharCount( 6 );
	
	m_pRadarAlpha->AddActionSignalTarget( this );
	m_pRadarScale->AddActionSignalTarget( this );
	m_pRadarSize->AddActionSignalTarget( this );
	
	m_pPlayerCountPos->AddActionSignalTarget( this );
	m_pRadarSquare->AddActionSignalTarget( this );
	m_pRadarRotate->AddActionSignalTarget( this );
	m_pXhairRainbow->AddActionSignalTarget( this );
	m_pXhairSniper->AddActionSignalTarget( this );
	m_pEnableColorHud->AddActionSignalTarget( this );
	m_pDisplayRadarName->AddActionSignalTarget( this );
	m_pDisplayRadarHealth->AddActionSignalTarget( this );
	m_pDisplayRadarLine->AddActionSignalTarget( this );
	m_pDisplayRoundtimerC4->AddActionSignalTarget( this );
	m_pHudColorEntry->AddActionSignalTarget( this );
	#ifdef ANDROID
    m_pKeyboardMouseMode->AddActionSignalTarget( this );
    #endif

	LoadControlSettings( "Resource/OptionGuiMenu.res" );

	#ifdef ANDROID
	if ( m_pKeyboardMouseMode )
		m_pKeyboardMouseMode->SetVisible( true );
	#endif
}

ClientModGuiMenu::~ClientModGuiMenu()
{
}

void ClientModGuiMenu::ApplyHudColor( int r, int g, int b )
{
	char szColor[32];
	Q_snprintf( szColor, sizeof( szColor ), "%d %d %d", r, g, b );
	ConVarRef( "hud_color" ).SetValue( szColor );
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
	
	m_pRadarAlpha->Reset();
	m_pRadarScale->Reset();
	m_pRadarSize->Reset();
	
	m_pRadarSquare->Reset();
	m_pRadarRotate->Reset();
	m_pXhairRainbow->Reset();
	m_pXhairSniper->Reset();
	m_pEnableColorHud->Reset();
	m_pDisplayRadarName->Reset();
	m_pDisplayRadarHealth->Reset();
	m_pDisplayRadarLine->Reset();
	m_pDisplayRoundtimerC4->Reset();
	#ifdef ANDROID
    m_pKeyboardMouseMode->Reset();
    #endif

	// Read "R G B" cvar and convert to RRGGBB hex for the text box
	int r = 255, g = 255, b = 255;
	sscanf( ConVarRef( "hud_color" ).GetString(), "%d %d %d", &r, &g, &b );
	char szHex[8];
	Q_snprintf( szHex, sizeof( szHex ), "%02X%02X%02X", r, g, b );
	m_pHudColorEntry->SetText( szHex );
}

void ClientModGuiMenu::OnApplyChanges()
{
	m_pPlayerCountPos->ApplyChanges();
	
	m_pRadarAlpha->ApplyChanges();
	m_pRadarScale->ApplyChanges();
	m_pRadarSize->ApplyChanges();
	
	m_pRadarSquare->ApplyChanges();
	m_pRadarRotate->ApplyChanges();
	m_pXhairRainbow->ApplyChanges();
	m_pXhairSniper->ApplyChanges();
	m_pEnableColorHud->ApplyChanges();
	m_pDisplayRadarName->ApplyChanges();
	m_pDisplayRadarHealth->ApplyChanges();
	m_pDisplayRadarLine->ApplyChanges();
	m_pDisplayRoundtimerC4->ApplyChanges();
	#ifdef ANDROID
    m_pKeyboardMouseMode->ApplyChanges();
    #endif

	// Parse RRGGBB hex from the text entry and apply
	char szHex[8];
	m_pHudColorEntry->GetText( szHex, sizeof( szHex ) );
	int r = 255, g = 255, b = 255;
	if ( strlen( szHex ) == 6 )
	{
		unsigned int hex = 0;
		sscanf( szHex, "%X", &hex );
		r = ( hex >> 16 ) & 0xFF;
		g = ( hex >> 8  ) & 0xFF;
		b =   hex         & 0xFF;
	}
	ApplyHudColor( r, g, b );
}
