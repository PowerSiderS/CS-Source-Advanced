#ifndef CLIENTMODMAINMENU_H
#define CLIENTMODMAINMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Label.h>

class CLabeledCommandComboBox;
class CCvarToggleCheckButton;
class CCvarSlider;

class ClientModMainMenu;

//-----------------------------------------------------------------------------
// Main Menu properties
//-----------------------------------------------------------------------------
class ClientModMainMenu: public vgui::PropertyPage
{
        DECLARE_CLASS_SIMPLE( ClientModMainMenu, vgui::PropertyPage );

public:
        ClientModMainMenu( vgui::Panel *parent );
        ~ClientModMainMenu();

        MESSAGE_FUNC( OnControlModified, "ControlModified" );
        MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );

        void UpdateViewmodelSliderLabels();

protected:
        MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", data );

        // Called when page is loaded.  Data should be reloaded from document into controls.
        virtual void OnResetData();
        // Called when the game UI is hidden (menu closed)
        virtual void OnGameUIHidden();
        // Called every frame to update the preview
        virtual void OnThink();
        // Called when the OK / Apply button is pressed.  Changed data should be written into document.
        virtual void OnApplyChanges();
        virtual void OnCommand( const char *command );
        MESSAGE_FUNC_CHARPTR( OnFileSelected, "FileSelected", fullpath );

private:
        void UpdateAvatarDisplay();
        double m_flNextAvatarUpdateTime;
        vgui::DHANDLE<vgui::FileOpenDialog> m_hImportSprayDialog;
        // Left column - Viewmodel Offset sliders
        CCvarSlider*                                    m_pViewmodelOffsetX;
        vgui::Label*                                    m_pViewmodelOffsetXLabel;
        CCvarSlider*                                    m_pViewmodelOffsetY;
        vgui::Label*                                    m_pViewmodelOffsetYLabel;
        CCvarSlider*                                    m_pViewmodelOffsetZ;
        vgui::Label*                                    m_pViewmodelOffsetZLabel;

        // Middle column - FOV and Recoil sliders
        CCvarSlider*                                    m_pViewmodelFOV;
        vgui::Label*                                    m_pViewmodelFOVLabel;
        CCvarSlider*                                    m_pViewmodelRecoil;
        vgui::Label*                                    m_pViewmodelRecoilLabel;

        // Right column - ComboBoxes
        CLabeledCommandComboBox*                m_pKillfeedType;                // cl_killfeed_csgo
        CLabeledCommandComboBox*                m_pViewbobStyle;                // cmod_new_bobbing
        CLabeledCommandComboBox*                m_pFlashlightType;              // r_rainbowflashlight
        CLabeledCommandComboBox*                m_pWeaponPos;                   // cl_righthand
        
        // New fields
        vgui::TextEntry*                                m_pNameEntry;
        vgui::TextEntry*                                m_pClanTagEntry;
        vgui::ImagePanel*                               m_pAvatarImage;
        vgui::Button*                                   m_pImportAvatarButton;
};

#endif // CLIENTMODMAINMENU_H
