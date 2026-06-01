#ifndef CLIENTMODCROSSHAIRMENU_H
#define CLIENTMODCROSSHAIRMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>

class CLabeledCommandComboBox;
class CCvarToggleCheckButton;
class CCvarSlider;
class CrosshairImagePanelMod;

class ClientModCrosshairMenu: public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( ClientModCrosshairMenu, vgui::PropertyPage );

public:
	ClientModCrosshairMenu( vgui::Panel *parent );
	~ClientModCrosshairMenu();

	void OnControlModified();

protected:
	virtual void OnResetData();
	virtual void OnApplyChanges();

private:
	CrosshairImagePanelMod*			m_pCrosshairImage;
};

#endif // CLIENTMODCROSSHAIRMENU_H
