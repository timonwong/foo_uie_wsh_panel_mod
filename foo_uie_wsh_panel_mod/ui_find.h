#pragma once

#include "resource.h"


class CDialogFind 
	: public CDialogImpl<CDialogFind>
	, public CDialogResize<CDialogFind>
{
private:
	int m_flags;
	pfc::string8 m_text;
	HWND m_hedit;

public:
	CDialogFind(HWND p_hedit) : m_hedit(p_hedit), m_flags(0)
	{
	}

	void FindResult(int pos);
	void OnFinalMessage(HWND hWnd);

public:
	enum { IDD = IDD_DIALOG_FIND };

	BEGIN_MSG_MAP(CDialogFind)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_ID_HANDLER_EX(IDC_FINDUP, OnFindUp)
		COMMAND_ID_HANDLER_EX(IDC_FINDDOWN, OnFindDown)
		COMMAND_HANDLER_EX(IDC_EDIT_FINDWHAT, EN_CHANGE, OnEditFindWhatEnChange)
		COMMAND_RANGE_HANDLER_EX(IDC_CHECK_MATCHCASE, IDC_CHECK_REGEXP, OnFlagCommand)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
		CHAIN_MSG_MAP(CDialogResize<CDialogFind>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CDialogFind)
		DLGRESIZE_CONTROL(IDC_EDIT_FINDWHAT, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_FINDUP, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_FINDDOWN, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()

public:
	LRESULT OnFindUp(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnFindDown(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnEditFindWhatEnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnFlagCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
};
