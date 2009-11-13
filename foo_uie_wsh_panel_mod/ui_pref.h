#pragma once

#include "resource.h"


class CDialogPref : public CDialogImpl<CDialogPref>
	, public CWinDataExchange<CDialogPref>
{
private:
	CListViewCtrl m_props;

public:
	void OnFinalMessage(HWND hWnd);
	void LoadProps(bool reset = false);
	void uGetItemText(int nItem, int nSubItem, pfc::string_base & out);

public:
	enum { IDD = IDD_DIALOG_PREFERENCE };

	BEGIN_MSG_MAP(CDialogPref)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		COMMAND_HANDLER_EX(IDC_BUTTON_EXPORT, BN_CLICKED, OnButtonExportBnClicked)
		COMMAND_HANDLER_EX(IDC_BUTTON_IMPORT, BN_CLICKED, OnButtonImportBnClicked)
		//COMMAND_HANDLER_EX(IDC_BUTTON_APPLY, BN_CLICKED, OnButtonApplyBnClicked)
		NOTIFY_HANDLER_EX(IDC_LIST_EDITOR_PROP, NM_DBLCLK, OnPropNMDblClk)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CDialogPref)
		DDX_CONTROL_HANDLE(IDC_LIST_EDITOR_PROP, m_props)
	END_DDX_MAP()

	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnDestroy();
	LRESULT OnPropNMDblClk(LPNMHDR pnmh);
	LRESULT OnButtonExportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnButtonImportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	//LRESULT OnButtonApplyBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
};

class wsh_preferences_page : public preferences_page
{
private:
	CDialogPref * dlg;

public:
	HWND create(HWND p_parent)
	{
		dlg = new CDialogPref;
		return dlg->Create(p_parent);
	}

	const char * get_name()
	{
		return "WSH Panel Mod";
	}

	GUID get_guid()
	{
		// {1624E0E0-049E-4927-B4DD-2DAF7FC2415F}
		static const GUID guid = 
		{ 0x1624e0e0, 0x49e, 0x4927, { 0xb4, 0xdd, 0x2d, 0xaf, 0x7f, 0xc2, 0x41, 0x5f } };

		return guid;
	}
	
	GUID get_parent_guid()
	{
		return guid_display;
	}

	bool reset_query()
	{
		return true;
	}

	void reset();
};