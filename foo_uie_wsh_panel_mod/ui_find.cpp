#include "stdafx.h"
#include "ui_find.h"


LRESULT CDialogFind::OnFindUp(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	int pos;

	if (m_text.is_empty())
		return 0;

	SendMessage(m_hedit, SCI_SEARCHANCHOR, 0, 0);
	pos = SendMessage(m_hedit, SCI_SEARCHPREV, m_flags, (LPARAM)m_text.get_ptr());
	FindResult(pos);
	return 0;
}

LRESULT CDialogFind::OnFindDown(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (m_text.is_empty())
		return 0;

	SendMessage(m_hedit, SCI_CHARRIGHT, 0, 0);
	SendMessage(m_hedit, SCI_SEARCHANCHOR, 0, 0);
	int pos = SendMessage(m_hedit, SCI_SEARCHNEXT, m_flags, (LPARAM)m_text.get_ptr());
	FindResult(pos);

	return 0;
}

void CDialogFind::FindResult(int pos)
{
	if (pos != -1)
	{
		// Scroll to view
		SendMessage(m_hedit, SCI_SCROLLCARET, 0, 0);
	}
	else
	{
		pfc::string8 temp = "Cannot find \"";

		temp += m_text;
		temp += "\"";
		uMessageBox(m_hWnd, temp.get_ptr(), WSPM_NAME, MB_ICONINFORMATION | MB_SETFOREGROUND);
	}
}

LRESULT CDialogFind::OnEditFindWhatEnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	uGetWindowText(GetDlgItem(IDC_EDIT_FINDWHAT), m_text);
	return 0;
}

LRESULT CDialogFind::OnFlagCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	bool check = uButton_GetCheck(m_hWnd, wID);
	int flag = 0;

	switch (wID)
	{
	case IDC_CHECK_MATCHCASE:
		flag = SCFIND_MATCHCASE;
		break;

	case IDC_CHECK_WHOLEWORD:
		flag = SCFIND_WHOLEWORD;
		break;

	case IDC_CHECK_WORDSTART:
		flag = SCFIND_WORDSTART;
		break;

	case IDC_CHECK_REGEXP:
		flag = SCFIND_REGEXP;
		break;
	}

	if (check)
		m_flags |= flag;
	else
		m_flags &= ~flag;

	return 0;
}

LRESULT CDialogFind::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	//DestroyWindow();
	ShowWindow(SW_HIDE);
	return 0;
}

void CDialogFind::OnFinalMessage(HWND hWnd)
{
	modeless_dialog_manager::g_remove(m_hWnd);
	delete this;
}

LRESULT CDialogFind::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	modeless_dialog_manager::g_add(m_hWnd);
	DlgResize_Init();
	m_find.SubclassWindow(GetDlgItem(IDC_EDIT_FINDWHAT), m_hWnd);
	return TRUE; // set focus to default control
}
