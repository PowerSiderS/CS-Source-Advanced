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
#include "vgui_controls/QueryBox.h"
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/FileOpenDialog.h>

#include "CvarTextEntry.h"
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
        m_pViewmodelOffsetX = new CCvarSlider( this, "ViewmodelOffsetXSlider", "", -5.0f, 5.0f, "viewmodel_offset_x" );
        m_pViewmodelOffsetXLabel = new Label( this, "ViewmodelOffsetXLabel", "" );
        m_pViewmodelOffsetY = new CCvarSlider( this, "ViewmodelOffsetYSlider", "", -5.0f, 5.0f, "viewmodel_offset_y" );
        m_pViewmodelOffsetYLabel = new Label( this, "ViewmodelOffsetYLabel", "" );
        m_pViewmodelOffsetZ = new CCvarSlider( this, "ViewmodelOffsetZSlider", "", -5.0f, 5.0f, "viewmodel_offset_z" );
        m_pViewmodelOffsetZLabel = new Label( this, "ViewmodelOffsetZLabel", "" );

        // Middle column - FOV and Recoil sliders
        m_pViewmodelFOV = new CCvarSlider( this, "ViewmodelFOVSlider", "", 54.0f, 90.0f, "viewmodel_fov" );
        m_pViewmodelFOVLabel = new Label( this, "ViewmodelFOVLabel", "" );
        m_pViewmodelRecoil = new CCvarSlider( this, "ViewmodelRecoilSlider", "", 0.0f, 2.0f, "viewmodel_recoil" );
        m_pViewmodelRecoilLabel = new Label( this, "ViewmodelRecoilLabel", "" );

        // Right column - ComboBoxes
        m_pKillfeedType = new CLabeledCommandComboBox( this, "KillfeedTypeComboBox" );
        m_pKillfeedType->AddItem( "CS: Source", "cl_killfeed_csgo 0" );
        m_pKillfeedType->AddItem( "CS: GO", "cl_killfeed_csgo 1" );

        m_pViewbobStyle = new CLabeledCommandComboBox( this, "ViewbobStyleComboBox" );
        m_pViewbobStyle->AddItem( "CS: Source", "cmod_new_bobbing 0" );
        m_pViewbobStyle->AddItem( "CS: GO", "cmod_new_bobbing 1" );

        m_pFlashlightType = new CLabeledCommandComboBox( this, "FlashlightTypeComboBox" );
        m_pFlashlightType->AddItem( "Normal", "r_rainbow_flashlight 0" );
        m_pFlashlightType->AddItem( "Rainbow", "r_rainbow_flashlight 1" );

        m_pWeaponPos = new CLabeledCommandComboBox( this, "WeaponPositionComboBox" );
        m_pWeaponPos->AddItem( "Left Hand", "cl_righthand 0" );
        m_pWeaponPos->AddItem( "Right Hand", "cl_righthand 1" );
        
        // New Fields
        m_pNameEntry = new TextEntry( this, "NameEntry" );
        m_pNameEntry->SetMaximumCharCount( 32 );
        m_pClanTagEntry = new TextEntry( this, "ClanTagEntry" );
        m_pClanTagEntry->SetMaximumCharCount( 12 );
        m_pAvatarImage = new ImagePanel( this, "AvatarImage" );
        m_pAvatarImage->SetShouldScaleImage( true );
        m_pImportAvatarButton = new Button( this, "ImportAvatarButton", "Import Avatar...", this, "ImportAvatar" );

        // Add action signal targets
        m_pViewmodelOffsetX->AddActionSignalTarget( this );
        m_pViewmodelOffsetY->AddActionSignalTarget( this );
        m_pViewmodelOffsetZ->AddActionSignalTarget( this );
        m_pViewmodelFOV->AddActionSignalTarget( this );
        m_pViewmodelRecoil->AddActionSignalTarget( this );
        m_pKillfeedType->AddActionSignalTarget( this );
        m_pViewbobStyle->AddActionSignalTarget( this );
        m_pFlashlightType->AddActionSignalTarget( this );
        m_pWeaponPos->AddActionSignalTarget( this );
        m_pNameEntry->AddActionSignalTarget( this );
        m_pClanTagEntry->AddActionSignalTarget( this );

        m_flNextAvatarUpdateTime = 0.0f;

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

void ClientModMainMenu::OnTextChanged( vgui::Panel *panel )
{
        OnControlModified();
}

void ClientModMainMenu::OnCommand( const char *command )
{
        if ( !Q_stricmp( command, "ImportAvatar" ) )
        {
                if ( !m_hImportSprayDialog.Get() )
                {
                        m_hImportSprayDialog = new FileOpenDialog( this, "Import Avatar", true );
                        m_hImportSprayDialog->AddFilter( "*.vtf", "VTF Files", true );
                        m_hImportSprayDialog->AddFilter( "*.jpg;*.png;*.tga", "Image Files", false );
                }
                m_hImportSprayDialog->DoModal( false );
        }
        else
        {
                BaseClass::OnCommand( command );
        }
}

void ClientModMainMenu::OnFileSelected( const char *fullpath )
{
        if ( !fullpath || !fullpath[0] )
                return;

        char materialsRelativePath[MAX_PATH];
        const char *materialsStart = strstr( fullpath, "materials" );
        
        if ( materialsStart )
        {
                Q_strncpy( materialsRelativePath, materialsStart, sizeof(materialsRelativePath) );
        }
        else
        {
                const char *pszFileName = Q_UnqualifiedFileName( fullpath );
                Q_snprintf( materialsRelativePath, sizeof(materialsRelativePath), "materials/%s", pszFileName );
        }
        
        if ( !Q_stristr( materialsRelativePath, ".vtf" ) )
        {
                Q_strncat( materialsRelativePath, ".vtf", sizeof(materialsRelativePath), COPY_ALL_CHARACTERS );
        }

        char cmd[512];
        Q_snprintf( cmd, sizeof(cmd), "cl_avatar \"%s\"\n", materialsRelativePath );
        engine->ClientCmd_Unrestricted( cmd );
        
        UpdateAvatarDisplay();
}

void ClientModMainMenu::UpdateAvatarDisplay()
{
        if ( !m_pAvatarImage )
                return;

        const char *avatarPath = cvar->FindVar("cl_avatar")->GetString();
        if ( !avatarPath || !avatarPath[0] )
        {
                m_pAvatarImage->SetImage( "" );
                return;
        }
        
        char materialName[256];
        const char *pszFileName = Q_UnqualifiedFileName(avatarPath);
        Q_strncpy(materialName, pszFileName, sizeof(materialName));

        // Strip extension
        char *ext = Q_stristr(materialName, ".vtf");
        if (ext) *ext = '\0';

        char fullLogoName[256];
        Q_snprintf(fullLogoName, sizeof(fullLogoName), "logos/%s", materialName);

        m_pAvatarImage->SetImage(fullLogoName);
        m_pAvatarImage->SetShouldScaleImage(true);
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

        // Set initial combo box values based on current cvar values
        ConVarRef cl_killfeed_csgo( "cl_killfeed_csgo" );
        if ( cl_killfeed_csgo.IsValid() )
                m_pKillfeedType->SetInitialItem( cl_killfeed_csgo.GetInt() );

        ConVarRef cmod_new_bobbing( "cmod_new_bobbing" );
        if ( cmod_new_bobbing.IsValid() )
                m_pViewbobStyle->SetInitialItem( cmod_new_bobbing.GetInt() );

        ConVarRef r_rainbowflashlight( "r_rainbow_flashlight" );
        if ( r_rainbowflashlight.IsValid() )
                m_pFlashlightType->SetInitialItem( r_rainbowflashlight.GetInt() );

        ConVarRef cl_righthand( "cl_righthand" );
        if ( cl_righthand.IsValid() )
                m_pWeaponPos->SetInitialItem( cl_righthand.GetInt() );
                
        // Set initial text entry values
        ConVarRef name( "name" );
        if ( name.IsValid() )
                m_pNameEntry->SetText( name.GetString() );
                
        ConVarRef cl_clantag( "cl_clantag" );
        if ( cl_clantag.IsValid() )
                m_pClanTagEntry->SetText( cl_clantag.GetString() );

        // Set initial avatar image
        UpdateAvatarDisplay();
}

void ClientModMainMenu::OnGameUIHidden()
{
        UpdateAvatarDisplay();
}

void ClientModMainMenu::OnThink()
{
        BaseClass::OnThink();

        double curtime = system()->GetTimeMillis() / 1000.0;
        if (curtime > m_flNextAvatarUpdateTime)
        {
                UpdateAvatarDisplay();
                m_flNextAvatarUpdateTime = curtime + 1.0; // Update every 1 second
        }
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
        m_pKillfeedType->ApplyChanges();
        m_pViewbobStyle->ApplyChanges();
        m_pFlashlightType->ApplyChanges();
        m_pWeaponPos->ApplyChanges();
        
        // Apply name and clantag
        char buf[64];
        m_pNameEntry->GetText( buf, sizeof(buf) );
        char cmd[128];
        Q_snprintf(cmd, sizeof(cmd), "name \"%s\"", buf);
        engine->ClientCmd_Unrestricted( cmd );
        
        m_pClanTagEntry->GetText( buf, sizeof(buf) );
        Q_snprintf(cmd, sizeof(cmd), "cl_clantag \"%s\"", buf);
        engine->ClientCmd_Unrestricted( cmd );
}
