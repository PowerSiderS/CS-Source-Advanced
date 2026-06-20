#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h> // SRC only!!
#endif

#include "ClientModMainMenu.h"
#include <stdio.h>

#include <vgui_controls/Button.h>
#include "tier1/KeyValues.h"
#include <vgui_controls/Label.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui_controls/ComboBox.h>

#include "CvarToggleCheckButton.h"
#include "cvarslider.h"
#include "LabeledCommandComboBox.h"
#include "EngineInterface.h"
#include "tier1/convar.h"

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
ClientModMainMenu::ClientModMainMenu( vgui::Panel *parent ): vgui::PropertyPage( parent, "ClientModMainMenu" )
{
	// Left column - Viewmodel Offset sliders (X, Y, Z)
	m_pViewmodelOffsetX      = new CCvarSlider( this, "ViewmodelOffsetXSlider", "", -5.0f, 5.0f, "viewmodel_offset_x" );
	m_pViewmodelOffsetXLabel = new Label( this, "ViewmodelOffsetXLabel", "" );
	m_pViewmodelOffsetY      = new CCvarSlider( this, "ViewmodelOffsetYSlider", "", -5.0f, 5.0f, "viewmodel_offset_y" );
	m_pViewmodelOffsetYLabel = new Label( this, "ViewmodelOffsetYLabel", "" );
	m_pViewmodelOffsetZ      = new CCvarSlider( this, "ViewmodelOffsetZSlider", "", -5.0f, 5.0f, "viewmodel_offset_z" );
	m_pViewmodelOffsetZLabel = new Label( this, "ViewmodelOffsetZLabel", "" );

	// Middle column - FOV and Recoil sliders
	m_pViewmodelFOV          = new CCvarSlider( this, "ViewmodelFOVSlider",    "", 54.0f, 90.0f, "viewmodel_fov" );
	m_pViewmodelFOVLabel     = new Label( this, "ViewmodelFOVLabel", "" );
	m_pViewmodelRecoil       = new CCvarSlider( this, "ViewmodelRecoilSlider", "", 0.0f,  2.0f,  "viewmodel_recoil" );
	m_pViewmodelRecoilLabel  = new Label( this, "ViewmodelRecoilLabel", "" );

	// Right column - ComboBoxes

	// Draw Tracers
	m_pDrawTracers = new CLabeledCommandComboBox( this, "KillfeedTypeComboBox" );
	m_pDrawTracers->AddItem( "Disabled", "r_drawtracers 0" );
	m_pDrawTracers->AddItem( "Enabled",  "r_drawtracers 1" );

	m_pViewbobStyle = new CLabeledCommandComboBox( this, "ViewbobStyleComboBox" );
	m_pViewbobStyle->AddItem( "CS: Source", "cmod_new_bobbing 0" );
	m_pViewbobStyle->AddItem( "CS: GO",     "cmod_new_bobbing 1" );

	m_pFlashlightType = new CLabeledCommandComboBox( this, "FlashlightTypeComboBox" );
	m_pFlashlightType->AddItem( "Normal",  "r_rainbow_flashlight 0" );
	m_pFlashlightType->AddItem( "Rainbow", "r_rainbow_flashlight 1" );

	m_pWeaponPos = new CLabeledCommandComboBox( this, "WeaponPositionComboBox" );
	m_pWeaponPos->AddItem( "Left Hand",  "cl_righthand 0" );
	m_pWeaponPos->AddItem( "Right Hand", "cl_righthand 1" );

	// Background selector
	m_pBackground = new CLabeledCommandComboBox( this, "BackgroundComboBox" );
	m_pBackground->AddItem( "Default",        "cl_background 1" );
	m_pBackground->AddItem( "The Final S3",   "cl_background 2" );
	m_pBackground->AddItem( "BF4 Animated",   "cl_background 3" );
	m_pBackground->AddItem( "Cyan Eyes",      "cl_background 4" );
	m_pBackground->AddItem( "Pillz Animated", "cl_background 5" );
	m_pBackground->AddItem( "Face Girl",      "cl_background 6" );

	// Add action signal targets
	m_pViewmodelOffsetX->AddActionSignalTarget( this );
	m_pViewmodelOffsetY->AddActionSignalTarget( this );
	m_pViewmodelOffsetZ->AddActionSignalTarget( this );
	m_pViewmodelFOV->AddActionSignalTarget( this );
	m_pViewmodelRecoil->AddActionSignalTarget( this );
	m_pDrawTracers->AddActionSignalTarget( this );
	m_pViewbobStyle->AddActionSignalTarget( this );
	m_pFlashlightType->AddActionSignalTarget( this );
	m_pWeaponPos->AddActionSignalTarget( this );
	m_pBackground->AddActionSignalTarget( this );

	LoadControlSettings( "Resource/OptionViewModel.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModMainMenu::~ClientModMainMenu()
{
}

void ClientModMainMenu::UpdateViewmodelSliderLabels()
{
	char strValue[8];
	Q_snprintf( strValue, sizeof( strValue ), "%2.1f", m_pViewmodelOffsetX->GetSliderValue() );
	m_pViewmodelOffsetXLabel->SetText( strValue );
	Q_snprintf( strValue, sizeof( strValue ), "%2.1f", m_pViewmodelOffsetY->GetSliderValue() );
	m_pViewmodelOffsetYLabel->SetText( strValue );
	Q_snprintf( strValue, sizeof( strValue ), "%2.1f", m_pViewmodelOffsetZ->GetSliderValue() );
	m_pViewmodelOffsetZLabel->SetText( strValue );
	Q_snprintf( strValue, sizeof( strValue ), "%2.1f", m_pViewmodelFOV->GetSliderValue() );
	m_pViewmodelFOVLabel->SetText( strValue );
	Q_snprintf( strValue, sizeof( strValue ), "%2.1f", m_pViewmodelRecoil->GetSliderValue() );
	m_pViewmodelRecoilLabel->SetText( strValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModMainMenu::OnControlModified()
{
	PostMessage( GetParent(), new KeyValues( "ApplyButtonEnable" ) );
	InvalidateLayout();
}

void ClientModMainMenu::OnSliderMoved( KeyValues *data )
{
	vgui::Panel* pPanel = static_cast<vgui::Panel*>(data->GetPtr( "panel" ));

	if ( pPanel == m_pViewmodelOffsetX || pPanel == m_pViewmodelOffsetY || pPanel == m_pViewmodelOffsetZ ||
		 pPanel == m_pViewmodelFOV || pPanel == m_pViewmodelRecoil )
	{
		UpdateViewmodelSliderLabels();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModMainMenu::OnResetData()
{
	m_pViewmodelOffsetX->Reset();
	m_pViewmodelOffsetY->Reset();
	m_pViewmodelOffsetZ->Reset();
	m_pViewmodelFOV->Reset();
	m_pViewmodelRecoil->Reset();
	UpdateViewmodelSliderLabels();

	ConVarRef r_drawtracers( "r_drawtracers" );
	if ( r_drawtracers.IsValid() )
		m_pDrawTracers->SetInitialItem( r_drawtracers.GetInt() );

	ConVarRef cmod_new_bobbing( "cmod_new_bobbing" );
	if ( cmod_new_bobbing.IsValid() )
		m_pViewbobStyle->SetInitialItem( cmod_new_bobbing.GetInt() );

	ConVarRef r_rainbowflashlight( "r_rainbow_flashlight" );
	if ( r_rainbowflashlight.IsValid() )
		m_pFlashlightType->SetInitialItem( r_rainbowflashlight.GetInt() );

	ConVarRef cl_righthand( "cl_righthand" );
	if ( cl_righthand.IsValid() )
		m_pWeaponPos->SetInitialItem( cl_righthand.GetInt() );

	ConVarRef cl_background( "cl_background" );
	if ( cl_background.IsValid() )
		m_pBackground->SetInitialItem( cl_background.GetInt() - 1 ); // cvar 1-6 -> index 0-5
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModMainMenu::OnApplyChanges()
{
	m_pViewmodelOffsetX->ApplyChanges();
	m_pViewmodelOffsetY->ApplyChanges();
	m_pViewmodelOffsetZ->ApplyChanges();
	m_pViewmodelFOV->ApplyChanges();
	m_pViewmodelRecoil->ApplyChanges();
	m_pDrawTracers->ApplyChanges();
	m_pViewbobStyle->ApplyChanges();
	m_pFlashlightType->ApplyChanges();
	m_pWeaponPos->ApplyChanges();
	m_pBackground->ApplyChanges();
}
