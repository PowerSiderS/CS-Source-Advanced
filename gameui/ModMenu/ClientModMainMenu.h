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

        void UpdateViewmodelSliderLabels();

protected:
        MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", data );

        // Called when page is loaded.  Data should be reloaded from document into controls.
        virtual void OnResetData();
        // Called when the OK / Apply button is pressed.  Changed data should be written into document.
        virtual void OnApplyChanges();

private:
        // Left column - Viewmodel Offset sliders
        CCvarSlider*                    m_pViewmodelOffsetX;
        vgui::Label*                    m_pViewmodelOffsetXLabel;
        CCvarSlider*                    m_pViewmodelOffsetY;
        vgui::Label*                    m_pViewmodelOffsetYLabel;
        CCvarSlider*                    m_pViewmodelOffsetZ;
        vgui::Label*                    m_pViewmodelOffsetZLabel;

        // Middle column - FOV and Recoil sliders
        CCvarSlider*                    m_pViewmodelFOV;
        vgui::Label*                    m_pViewmodelFOVLabel;
        CCvarSlider*                    m_pViewmodelRecoil;
        vgui::Label*                    m_pViewmodelRecoilLabel;

        // Right column - ComboBoxes
        CLabeledCommandComboBox*        m_pDrawTracers;     // r_drawtracers
        CLabeledCommandComboBox*        m_pViewbobStyle;    // cmod_new_bobbing
        CLabeledCommandComboBox*        m_pFlashlightType;  // r_rainbowflashlight
        CLabeledCommandComboBox*        m_pWeaponPos;       // cl_righthand
        CLabeledCommandComboBox*        m_pBackground;      // cl_background
};

#endif // CLIENTMODMAINMENU_H
