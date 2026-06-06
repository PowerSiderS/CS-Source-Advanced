//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Unified Options dialog — sidebar categories + horizontal sub-tabs
//
//=============================================================================//

#include "BasePanel.h"
#include "OptionsDialog.h"

#include "vgui_controls/Button.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/Label.h"

#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"

#include "KeyValues.h"
#include "OptionsSubKeyboard.h"
#include "OptionsSubMouse.h"
#include "OptionsSubTouch.h"
#include "OptionsSubAudio.h"
#include "OptionsSubVideo.h"
#include "OptionsSubVoice.h"
#include "OptionsSubMultiplayer.h"
#include "OptionsSubDifficulty.h"
#include "OptionsSubPortal.h"
#ifdef WIN32
#include "OptionsSubHaptics.h"
#endif
#include "ModInfo.h"

#include "ModMenu/ClientModMainMenu.h"
#include "ModMenu/ClientModCrosshairMenu.h"
#include "ModMenu/ClientModGuiMenu.h"
#include "ModMenu/ClientModGyroMenu.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// Sidebar category indices
enum EOptionsCategory
{
        CAT_CONTROLS  = 0,
        CAT_SOUNDS    = 1,
        CAT_VIDEO     = 2,
        CAT_GAMEPLAY  = 3,
        CAT_COUNT     = 4,
};

// Sidebar dimensions
static const int SIDEBAR_W    = 108;
static const int SIDEBAR_PAD  = 5;
static const int BTN_H        = 28;
static const int BTN_SPACING  = 34;
static const int SHEET_GAP    = 5;
static const int BOTTOM_H     = 34;  // height reserved for OK/Cancel/Apply row

//-----------------------------------------------------------------------------
COptionsDialog::COptionsDialog( vgui::Panel *parent )
        : vgui::Frame( parent, "OptionsDialog" )
        , m_iActiveCategory( -1 )
        , m_pOptionsSubMultiplayer( nullptr )
        , m_pOptionsSubAudio( nullptr )
        , m_pOptionsSubVideo( nullptr )
{
        SetDeleteSelfOnClose( true );
        SetSizeable( false );
        SetTitle( "#GameUI_Options", true );

        int w = 610, h = 400;
        if ( IsProportional() )
        {
                w = scheme()->GetProportionalScaledValueEx( GetScheme(), w );
                h = scheme()->GetProportionalScaledValueEx( GetScheme(), h );
        }
        SetBounds( 0, 0, w, h );

        // ----------------------------------------------------------------
        // Sidebar category buttons
        // ----------------------------------------------------------------
        m_pBtnControls  = new Button( this, "BtnControls",  "Controls",  this, "SelectControls"  );
        m_pBtnSounds    = new Button( this, "BtnSounds",    "Sounds",    this, "SelectSounds"    );
        m_pBtnVideo     = new Button( this, "BtnVideo",     "Video",     this, "SelectVideo"     );
        m_pBtnGameplay  = new Button( this, "BtnGameplay",  "Gameplay",  this, "SelectGameplay"  );

        // ----------------------------------------------------------------
        // Create one PropertySheet per category
        // ----------------------------------------------------------------
        m_pSheetControls  = new PropertySheet( this, "SheetControls"  );
        m_pSheetSounds    = new PropertySheet( this, "SheetSounds"    );
        m_pSheetVideo     = new PropertySheet( this, "SheetVideo"     );
        m_pSheetGameplay  = new PropertySheet( this, "SheetGameplay"  );

        // ---- Controls: Keyboard | Mouse | Touch ----
        m_pSheetControls->AddPage( new COptionsSubKeyboard( this ), "#GameUI_Keyboard" );
        m_pSheetControls->AddPage( new COptionsSubMouse( this ),    "#GameUI_Mouse"    );
#ifdef ANDROID
        m_pSheetControls->AddPage( new COptionsSubTouch( this ),    "Touch"            );
#endif
        m_pSheetControls->SetTabWidth( 84 );

        // ---- Sounds: Audio | Voice ----
        m_pOptionsSubAudio = new COptionsSubAudio( this );
        m_pSheetSounds->AddPage( m_pOptionsSubAudio, "#GameUI_Audio" );
        if ( !ModInfo().IsSinglePlayerOnly() )
                m_pSheetSounds->AddPage( new COptionsSubVoice( this ), "#GameUI_Voice" );
        m_pSheetSounds->SetTabWidth( 84 );

        // ---- Video: ViewModel | Video | Gyroscope ----
        m_pSheetVideo->AddPage( new ClientModMainMenu( this ),  "#GameUI_ClientModMain" );
        m_pOptionsSubVideo = new COptionsSubVideo( this );
        m_pSheetVideo->AddPage( m_pOptionsSubVideo, "#GameUI_Video" );
        m_pSheetVideo->AddPage( new ClientModGyroMenu( this ),  "#GameUI_ClientModGyro" );
        m_pSheetVideo->SetTabWidth( 84 );

        // ---- Gameplay: Crosshair | Multiplayer | GUI ----
        m_pSheetGameplay->AddPage( new ClientModCrosshairMenu( this ), "#GameUI_ClientModCrosshair" );
        if ( !ModInfo().IsSinglePlayerOnly() || ModInfo().IsMultiplayerOnly() )
        {
                m_pOptionsSubMultiplayer = new COptionsSubMultiplayer( this );
                m_pSheetGameplay->AddPage( m_pOptionsSubMultiplayer, "#GameUI_Multiplayer" );
        }
        m_pSheetGameplay->AddPage( new ClientModGuiMenu( this ), "GUI" );
        m_pSheetGameplay->SetTabWidth( 84 );

        // Haptics (Windows-only, added to Controls sheet)
#if defined( WIN32 ) && !defined( _X360 )
        ConVarRef checkHap( "hap_HasDevice" );
        checkHap.Init( "hap_HasDevice", true );
        if ( checkHap.GetBool() )
                m_pSheetControls->AddPage( new COptionsSubHaptics( this ), "#GameUI_Haptics_TabTitle" );
#endif

        // ----------------------------------------------------------------
        // Bottom buttons
        // ----------------------------------------------------------------
        m_pOKButton     = new Button( this, "OK",     "#GameUI_OK",     this, "OK"     );
        m_pCancelButton = new Button( this, "Cancel", "#GameUI_Cancel", this, "Cancel" );
        m_pApplyButton  = new Button( this, "Apply",  "#GameUI_Apply",  this, "Apply"  );
        m_pApplyButton->SetEnabled( false );

        // Start on Controls
        SelectCategory( CAT_CONTROLS );
}

//-----------------------------------------------------------------------------
COptionsDialog::~COptionsDialog()
{
}

//-----------------------------------------------------------------------------
void COptionsDialog::PerformLayout()
{
        BaseClass::PerformLayout();

        int w = GetWide();
        int h = GetTall();
        int captionH = GetCaptionHeight();   // title bar height (~28px)

        int contentY = captionH + SIDEBAR_PAD;
        int contentH = h - captionH - SIDEBAR_PAD - BOTTOM_H;

        // Sidebar buttons – stacked vertically on the left
        for ( int i = 0; i < CAT_COUNT; i++ )
        {
                vgui::Button *btn = nullptr;
                switch ( i )
                {
                case CAT_CONTROLS: btn = m_pBtnControls; break;
                case CAT_SOUNDS:   btn = m_pBtnSounds;   break;
                case CAT_VIDEO:    btn = m_pBtnVideo;     break;
                case CAT_GAMEPLAY: btn = m_pBtnGameplay;  break;
                }
                if ( btn )
                        btn->SetBounds( SIDEBAR_PAD, contentY + i * BTN_SPACING, SIDEBAR_W - SIDEBAR_PAD * 2, BTN_H );
        }

        // PropertySheets – fill the right portion
        int sheetX = SIDEBAR_W + SHEET_GAP;
        int sheetW = w - sheetX - SIDEBAR_PAD;
        m_pSheetControls ->SetBounds( sheetX, contentY, sheetW, contentH );
        m_pSheetSounds   ->SetBounds( sheetX, contentY, sheetW, contentH );
        m_pSheetVideo    ->SetBounds( sheetX, contentY, sheetW, contentH );
        m_pSheetGameplay ->SetBounds( sheetX, contentY, sheetW, contentH );

        // OK / Cancel / Apply at bottom-right
        int btnW  = 80;
        int btnH  = 24;
        int btnY  = h - BOTTOM_H + ( BOTTOM_H - btnH ) / 2;
        m_pApplyButton ->SetBounds( w - SIDEBAR_PAD - btnW,                       btnY, btnW, btnH );
        m_pCancelButton->SetBounds( w - SIDEBAR_PAD - btnW * 2 - SIDEBAR_PAD,     btnY, btnW, btnH );
        m_pOKButton    ->SetBounds( w - SIDEBAR_PAD - btnW * 3 - SIDEBAR_PAD * 2, btnY, btnW, btnH );
}

//-----------------------------------------------------------------------------
void COptionsDialog::SelectCategory( int iCategory )
{
        if ( iCategory == m_iActiveCategory )
                return;

        // Hide all sheets; show the requested one
        m_pSheetControls ->SetVisible( false );
        m_pSheetSounds   ->SetVisible( false );
        m_pSheetVideo    ->SetVisible( false );
        m_pSheetGameplay ->SetVisible( false );

        // Re-enable all sidebar buttons, then disable the active one
        m_pBtnControls ->SetEnabled( true );
        m_pBtnSounds   ->SetEnabled( true );
        m_pBtnVideo    ->SetEnabled( true );
        m_pBtnGameplay ->SetEnabled( true );

        vgui::PropertySheet *pSheet = nullptr;
        vgui::Button        *pBtn   = nullptr;

        switch ( iCategory )
        {
        case CAT_CONTROLS: pSheet = m_pSheetControls; pBtn = m_pBtnControls; break;
        case CAT_SOUNDS:   pSheet = m_pSheetSounds;   pBtn = m_pBtnSounds;   break;
        case CAT_VIDEO:    pSheet = m_pSheetVideo;     pBtn = m_pBtnVideo;    break;
        case CAT_GAMEPLAY: pSheet = m_pSheetGameplay;  pBtn = m_pBtnGameplay; break;
        default: break;
        }

        if ( pSheet ) pSheet->SetVisible( true );
        if ( pBtn   ) pBtn->SetEnabled( false );  // "selected" appearance

        m_iActiveCategory = iCategory;
}

//-----------------------------------------------------------------------------
void COptionsDialog::ApplyAllChanges()
{
        m_pSheetControls ->ApplyChanges();
        m_pSheetSounds   ->ApplyChanges();
        m_pSheetVideo    ->ApplyChanges();
        m_pSheetGameplay ->ApplyChanges();
}

//-----------------------------------------------------------------------------
void COptionsDialog::OnCommand( const char *command )
{
        if ( !Q_stricmp( command, "SelectControls" ) )
        {
                SelectCategory( CAT_CONTROLS );
        }
        else if ( !Q_stricmp( command, "SelectSounds" ) )
        {
                SelectCategory( CAT_SOUNDS );
        }
        else if ( !Q_stricmp( command, "SelectVideo" ) )
        {
                SelectCategory( CAT_VIDEO );
        }
        else if ( !Q_stricmp( command, "SelectGameplay" ) )
        {
                SelectCategory( CAT_GAMEPLAY );
        }
        else if ( !Q_stricmp( command, "OK" ) )
        {
                ApplyAllChanges();
                Close();
        }
        else if ( !Q_stricmp( command, "Apply" ) )
        {
                ApplyAllChanges();
                m_pApplyButton->SetEnabled( false );
        }
        else if ( !Q_stricmp( command, "Cancel" ) )
        {
                Close();
        }
        else
        {
                BaseClass::OnCommand( command );
        }
}

//-----------------------------------------------------------------------------
void COptionsDialog::OnApplyButtonEnable()
{
        EnableApplyButton( true );
}

void COptionsDialog::OnControlModified()
{
        EnableApplyButton( true );
}

void COptionsDialog::EnableApplyButton( bool bEnable )
{
        if ( m_pApplyButton )
                m_pApplyButton->SetEnabled( bEnable );
}

//-----------------------------------------------------------------------------
void COptionsDialog::Activate()
{
        BaseClass::Activate();
        EnableApplyButton( false );
}

//-----------------------------------------------------------------------------
void COptionsDialog::Run()
{
        SetTitle( "#GameUI_Options", true );
        Activate();
}

//-----------------------------------------------------------------------------
void COptionsDialog::OnKeyCodePressed( KeyCode code )
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
void COptionsDialog::NavigateToMultiplayer()
{
        SelectCategory( CAT_GAMEPLAY );
        if ( m_pOptionsSubMultiplayer )
                m_pSheetGameplay->SetActivePage( m_pOptionsSubMultiplayer );
}

void COptionsDialog::OnGameUIHidden()
{
        for ( int i = 0; i < GetChildCount(); i++ )
        {
                Panel *pChild = GetChild( i );
                if ( pChild )
                        PostMessage( pChild, new KeyValues( "GameUIHidden" ) );
        }
}
