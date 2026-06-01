#ifndef CLIENTMODGUIMENU_H
#define CLIENTMODGUIMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Label.h>

class CLabeledCommandComboBox;
class CCvarToggleCheckButton;
class CCvarSlider;

class ClientModGuiMenu : public vgui::PropertyPage
{
        DECLARE_CLASS_SIMPLE( ClientModGuiMenu, vgui::PropertyPage );

public:
        ClientModGuiMenu( vgui::Panel *parent );
        ~ClientModGuiMenu();

protected:
        virtual void OnResetData();
        virtual void OnApplyChanges();
        MESSAGE_FUNC( OnControlModified, "ControlModified" );
        MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
        MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", data );
        MESSAGE_FUNC( OnCheckButtonChecked, "CheckButtonChecked" );

private:
        CLabeledCommandComboBox*        m_pHudStyle;
        CLabeledCommandComboBox*        m_pPlayerCountPos;
        CLabeledCommandComboBox*        m_pHudColor;

        CCvarToggleCheckButton*         m_pRadarSquare;
        CCvarSlider*                            m_pRadarAlpha;
        CCvarToggleCheckButton*         m_pRadarRotate;
        CCvarSlider*                            m_pRadarScale;
        CCvarToggleCheckButton*         m_pXhairRainbow;
};

#endif // CLIENTMODGUIMENU_H
