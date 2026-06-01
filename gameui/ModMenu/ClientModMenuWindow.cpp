//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BasePanel.h"
#include "ClientModMenuWindow.h"

#include "vgui_controls/Button.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/QueryBox.h"

#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"

#include "KeyValues.h"

#include "ClientModMenuWindow.h"
#include "ClientModMainMenu.h"
#include "ClientModCrosshairMenu.h"
#include "ClientModGuiMenu.h"

using namespace vgui;

#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
ClientModMenuWindow::ClientModMenuWindow(vgui::Panel *parent) : PropertyDialog(parent, "ClientModMenuWindow")
{
        int w = 512;
        int h = 406;
        
        if (IsProportional())
        {
        	w = scheme()->GetProportionalScaledValueEx(GetScheme(), w);
		    h = scheme()->GetProportionalScaledValueEx(GetScheme(), h);
		}
	
        SetDeleteSelfOnClose(true);
        SetBounds(0, 0, w, h);
        SetMoveable( true );

        SetTitle("#GameUI_Clientmod_Menu_Window", true);

        AddPage(new ClientModMainMenu(this), "#GameUI_ClientModMain");
        AddPage(new ClientModCrosshairMenu(this), "#GameUI_ClientModCrosshair");
        AddPage(new ClientModGuiMenu(this), "GUI");

        SetApplyButtonVisible(true);
        GetPropertySheet()->SetTabWidth(84);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
ClientModMenuWindow::~ClientModMenuWindow()
{
}
/*
//-----------------------------------------------------------------------------
// Purpose: Centers and sizes the dialog based on screen resolution
//-----------------------------------------------------------------------------
void ClientModMenuWindow::PerformLayout()
{
    BaseClass::PerformLayout();

    int screenW, screenH;
    vgui::surface()->GetScreenSize( screenW, screenH );

    int dialogW = screenW * 660 / 1366; 
    int dialogH = screenH * 500 / 630; 

    // Adjust position to stay centered and fit resolution
    SetSize( dialogW, dialogH );
    
    // Ensure the menu stays within screen bounds if resolution is very low
    if (dialogW > screenW) dialogW = screenW - 20;
    if (dialogH > screenH) dialogH = screenH - 20;
    SetSize(dialogW, dialogH);

    MoveToCenterOfScreen();
}
*/
//-----------------------------------------------------------------------------
// Purpose: Brings the dialog to the fore
//-----------------------------------------------------------------------------
void ClientModMenuWindow::Activate()
{
        BaseClass::Activate();
        EnableApplyButton(false);
}

void ClientModMenuWindow::OnKeyCodePressed( KeyCode code )
{
        switch ( GetBaseButtonCode( code ) )
        {
        case KEY_XBUTTON_B:
                OnCommand( "Cancel" );
                return;
        }

        BaseClass::OnKeyCodePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: Opens the dialog
//-----------------------------------------------------------------------------
void ClientModMenuWindow::Run()
{
        SetTitle("#GameUI_ClientModMenuTitle", true);
        Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the GameUI is hidden
//-----------------------------------------------------------------------------
void ClientModMenuWindow::OnGameUIHidden()
{
        for ( int i = 0 ; i < GetChildCount() ; i++ )
        {
                Panel *pChild = GetChild( i );
                if ( pChild )
                {
                        PostMessage( pChild, new KeyValues( "GameUIHidden" ) );
                }
        }
}
