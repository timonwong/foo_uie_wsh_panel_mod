#pragma once

#include "config.h"
#include "resource.h"
#include "PropertyList.h"


// Forward declarations
class uie_win;


class CDialogProperty
	: public CDialogImpl<CDialogProperty>
	, public CDialogResize<CDialogProperty>
{
private:
	uie_win * m_parent;
	CPropertyListCtrl m_properties;
	sci_prop_config::t_map m_dup_prop_map;

public:
	CDialogProperty(uie_win * p_parent) : m_parent(p_parent)
	{
	}

	void LoadProperties(bool reload = true);
	void Apply();

public:
	enum { IDD = IDD_DIALOG_PROPERTIES };

	BEGIN_MSG_MAP(CDialogProperty)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_RANGE_HANDLER_EX(IDOK, IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER_EX(IDAPPLY, OnCloseCmd)
		COMMAND_HANDLER_EX(IDC_CLEARALL, BN_CLICKED, OnClearallBnClicked)
		COMMAND_HANDLER_EX(IDC_DEL, BN_CLICKED, OnDelBnClicked)
		COMMAND_HANDLER_EX(IDC_IMPORT, BN_CLICKED, OnImportBnClicked)
		COMMAND_HANDLER_EX(IDC_EXPORT, BN_CLICKED, OnExportBnClicked)
		NOTIFY_CODE_HANDLER_EX(PIN_ITEMCHANGED, OnPinItemChanged)
		CHAIN_MSG_MAP(CDialogResize<CDialogProperty>)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CDialogProperty)
		DLGRESIZE_CONTROL(IDC_LIST_PROPERTIES, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_DEL, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CLEARALL, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_IMPORT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_EXPORT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDAPPLY, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

public:
	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnPinItemChanged(LPNMHDR pnmh);
	LRESULT OnClearallBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnDelBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnImportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnExportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
};


class menu_node_properties : public ui_extension::menu_node_command_t
{
private:
	uie_win * p_this;

public:
	menu_node_properties(uie_win * wnd) : p_this(wnd) {};

	bool get_display_data(pfc::string_base & p_out,unsigned & p_displayflags) const
	{
		p_out = "Properties";
		p_displayflags= 0;
		return true;
	}

	bool get_description(pfc::string_base & p_out) const
	{
		return false;
	}

	void execute();
};
