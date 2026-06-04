//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Unified options dialog with sidebar categories + horizontal sub-tabs
//
//=============================================================================//

#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"
#include "vgui_controls/KeyRepeat.h"

//-----------------------------------------------------------------------------
// Purpose: Unified Options dialog
//   Left sidebar: Controls | Sounds | Video | Gameplay  (vertical buttons)
//   Right area:   PropertySheet whose pages swap per category
//-----------------------------------------------------------------------------
class COptionsDialog : public vgui::Frame
{
        DECLARE_CLASS_SIMPLE( COptionsDialog, vgui::Frame );

public:
        COptionsDialog( vgui::Panel *parent );
        ~COptionsDialog();

        void Run();
        virtual void Activate();
        virtual void PerformLayout();

        void OnKeyCodePressed( vgui::KeyCode code );

        // External accessor kept for BasePanel compatibility
        vgui::PropertyPage* GetOptionsSubMultiplayer( void ) { return m_pOptionsSubMultiplayer; }

        // Navigate directly to the Multiplayer sub-tab (used by refresh_options_dialog)
        void NavigateToMultiplayer();

        void EnableApplyButton( bool bEnable );
        void ApplyChanges() { ApplyAllChanges(); }

        MESSAGE_FUNC( OnGameUIHidden,     "GameUIHidden"     );
        MESSAGE_FUNC( OnApplyButtonEnable,"ApplyButtonEnable");
        MESSAGE_FUNC( OnControlModified,  "ControlModified"  );

        virtual void OnCommand( const char *command );

private:
        void SelectCategory( int iCategory );
        void ApplyAllChanges();

        // ---- sidebar buttons ----
        vgui::Button *m_pBtnControls;
        vgui::Button *m_pBtnSounds;
        vgui::Button *m_pBtnVideo;
        vgui::Button *m_pBtnGameplay;

        // ---- per-category PropertySheets ----
        vgui::PropertySheet *m_pSheetControls;   // Keyboard | Touch | Mouse
        vgui::PropertySheet *m_pSheetSounds;     // Audio | Voice
        vgui::PropertySheet *m_pSheetVideo;      // ViewModel | Video
        vgui::PropertySheet *m_pSheetGameplay;   // Crosshair | Multiplayer | GUI

        int m_iActiveCategory;  // 0-3

        // ---- bottom buttons ----
        vgui::Button *m_pOKButton;
        vgui::Button *m_pCancelButton;
        vgui::Button *m_pApplyButton;

        // ---- kept for external access ----
        vgui::PropertyPage      *m_pOptionsSubMultiplayer;
        class COptionsSubAudio  *m_pOptionsSubAudio;
        class COptionsSubVideo  *m_pOptionsSubVideo;
};


// ---- Xbox variant (unchanged) ----

#define OPTIONS_MAX_NUM_ITEMS 15

struct OptionData_t;

class COptionsDialogXbox : public vgui::Frame
{
        DECLARE_CLASS_SIMPLE( COptionsDialogXbox, vgui::Frame );

public:
        COptionsDialogXbox( vgui::Panel *parent, bool bControllerOptions = false );
        ~COptionsDialogXbox();

        virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
        virtual void ApplySettings( KeyValues *inResourceData );
        virtual void OnClose();
        virtual void OnKeyCodePressed( vgui::KeyCode code );
        virtual void OnCommand( const char *command );
        virtual void OnKeyCodeReleased( vgui::KeyCode code );
        virtual void OnThink();

private:
        void HandleInactiveKeyCodePressed( vgui::KeyCode code );
        void HandleActiveKeyCodePressed( vgui::KeyCode code );
        void HandleBindKeyCodePressed( vgui::KeyCode code );

        int  GetSelectionLabel( void ) { return m_iSelection - m_iScroll; }

        void ActivateSelection( void );
        void DeactivateSelection( void );
        void ChangeSelection( int iChange );
        void UpdateFooter( void );
        void UpdateSelection( void );
        void UpdateScroll( void );
        void UncacheChoices( void );
        void GetChoiceFromConvar( OptionData_t *pOption );
        void ChangeValue( float fChange );
        void UnbindOption( OptionData_t *pOption, int iLabel );
        void UpdateValue( OptionData_t *pOption, int iLabel );
        void UpdateBind( OptionData_t *pOption, int iLabel,
                         ButtonCode_t codeIgnore = BUTTON_CODE_INVALID,
                         ButtonCode_t codeAdd    = BUTTON_CODE_INVALID );
        void UpdateAllBinds( ButtonCode_t code );
        void FillInDefaultBindings( void );
        bool ShouldSkipOption( KeyValues *pKey );
        void ReadOptionsFromFile( const char *pchFileName );
        void SortOptions( void );
        void InitializeSliderDefaults( void );

private:
        bool m_bControllerOptions;
        bool m_bOptionsChanged;
        bool m_bOldForceEnglishAudio;

        CFooterPanel *m_pFooter;

        CUtlVector<OptionData_t*> *m_pOptions;

        bool         m_bSelectionActive;
        OptionData_t *m_pSelectedOption;

        int  m_iSelection;
        int  m_iScroll;
        int  m_iSelectorYStart;
        int  m_iOptionSpacing;
        int  m_iNumItems;
        int  m_iXAxisState;
        int  m_iYAxisState;
        float m_fNextChangeTime;

        vgui::Panel *m_pOptionsSelectionLeft;
        vgui::Panel *m_pOptionsSelectionLeft2;
        vgui::Label *m_pOptionsUpArrow;
        vgui::Label *m_pOptionsDownArrow;

        vgui::Label      *(m_pOptionLabels[ OPTIONS_MAX_NUM_ITEMS ]);
        vgui::Label      *(m_pValueLabels [ OPTIONS_MAX_NUM_ITEMS ]);
        vgui::AnalogBar  *(m_pValueBars   [ OPTIONS_MAX_NUM_ITEMS ]);

        vgui::HFont m_hLabelFont;
        vgui::HFont m_hButtonFont;

        Color m_SelectedColor;

        vgui::CKeyRepeatHandler m_KeyRepeat;

        int m_nButtonGap;
};

#endif // OPTIONSDIALOG_H
