#include "stdafx.h"
#include "ui_pref.h"
#include "scintilla_prop_sets.h"
#include "global_cfg.h"
#include "ui_name_value_edit.h"
#include "ui_conf.h"
#include "helpers.h"

namespace 
{
	static preferences_page_factory_t<wsh_preferences_page> g_pref;
}


LRESULT CDialogPref::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	// Set Spin	
	SendDlgItemMessage(IDC_SPIN_TIMEOUT, UDM_SETRANGE, 0, MAKELONG(180, 0));
	SendDlgItemMessage(IDC_SPIN_TIMEOUT, UDM_SETPOS, 0, MAKELONG(g_cfg_timeout.get_value(), 0));

	// Check "Safe mode"
	SendDlgItemMessage(IDC_CHECK_SAFE_MODE, BM_SETCHECK, g_cfg_safe_mode ? BST_CHECKED : BST_UNCHECKED, 0);

	DoDataExchange();

	// Enable Visual Style
	if (helpers::get_is_vista_or_later())
		SetWindowTheme(m_props.m_hWnd, L"explorer", NULL);

	// Show Grid lines (XP only)
	m_props.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | (helpers::get_is_vista_or_later() ? LVS_EX_DOUBLEBUFFER : LVS_EX_GRIDLINES));
	m_props.AddColumn(_T("Name"), 0);
	m_props.SetColumnWidth(0, 140);
	m_props.AddColumn(_T("Value"), 1);
	m_props.SetColumnWidth(1, 260);
	LoadProps();

	return TRUE; // set focus to default control
}

void CDialogPref::OnFinalMessage(HWND hWnd)
{
	delete this;
}

LRESULT CDialogPref::OnDestroy()
{
	BOOL translated;
	int val = GetDlgItemInt(IDC_EDIT_TIMEOUT, &translated, FALSE);

	if (translated) 
		g_cfg_timeout = val;

	g_cfg_safe_mode = (SendDlgItemMessage(IDC_CHECK_SAFE_MODE, BM_GETCHECK) == BST_CHECKED);

	return 0;
}

void CDialogPref::LoadProps(bool reset /*= false*/)
{
	if (reset)
		g_sci_prop_sets.reset();

	pfc::stringcvt::string_os_from_utf8_fast conv;
	t_sci_prop_set_list & prop_sets = g_sci_prop_sets.val();
	
	m_props.DeleteAllItems();

	for (t_size i = 0; i < prop_sets.get_count(); ++i)
	{
		conv.convert(prop_sets[i].key);
		m_props.AddItem(i, 0, conv);

		conv.convert(prop_sets[i].val);
		m_props.AddItem(i, 1, conv);
	}
}

void wsh_preferences_page::reset()
{
	if (!dlg)
		return;

	dlg->LoadProps(true);
}

LRESULT CDialogPref::OnPropNMDblClk(LPNMHDR pnmh)
{
	//for ListView - (LPNMITEMACTIVATE)pnmh
	//for StatusBar	- (LPNMMOUSE)pnmh
	LPNMITEMACTIVATE pniv = (LPNMITEMACTIVATE)pnmh;

	if (pniv->iItem >= 0)
	{
		t_sci_prop_set_list & prop_sets = g_sci_prop_sets.val();
		pfc::string8 key, val;
	
		uGetItemText(pniv->iItem, 0, key);
		uGetItemText(pniv->iItem, 1, val);

		CNameValueEdit dlg(key, val);
		
		if (IDOK == dlg.DoModal(m_hWnd))
		{
			dlg.GetValue(val);

			// Save
			for (t_size i = 0; i < prop_sets.get_count(); ++i)
			{
				if (strcmp(prop_sets[i].key, key) == 0)
				{
					prop_sets[i].val = val;
					break;
				}
			}

			// Update list
			m_props.SetItemText(pniv->iItem, 1, pfc::stringcvt::string_wide_from_utf8_fast(val));
			DoDataExchange();
		}
	}

	return 0;
}

void CDialogPref::uGetItemText(int nItem, int nSubItem, pfc::string_base & out)
{
	enum { BUFFER_LEN = 1024 };
	TCHAR buffer[BUFFER_LEN];

	m_props.GetItemText(nItem, nSubItem, buffer, BUFFER_LEN);
	out.set_string(pfc::stringcvt::string_utf8_from_os(buffer));
}

LRESULT CDialogPref::OnButtonExportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pfc::string8_fast filename;

	if (uGetOpenFileName(m_hWnd, "Configuration files|*.cfg", 0, "cfg", "Save as", NULL, filename, TRUE))
		g_sci_prop_sets.export_to_file(filename);

	return 0;
}

LRESULT CDialogPref::OnButtonImportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pfc::string8_fast filename;

	if (uGetOpenFileName(m_hWnd, "Configuration files|*.cfg|All files|*.*", 0, "cfg", "Import from", NULL, filename, FALSE))
		g_sci_prop_sets.import_from_file(filename);

	LoadProps();
	return 0;
}

//LRESULT CDialogPref::OnButtonApplyBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
//{
//	g_config_dlg_mgr.PostMessage(UM_DIALOG_STYLE_RELOAD, 0, 0);
//	return 0;
//}
