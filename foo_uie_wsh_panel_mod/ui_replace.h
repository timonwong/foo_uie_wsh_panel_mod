#pragma once

#include "resource.h"


class CDialogReplace
	: public CDialogImpl<CDialogReplace>
	, public CDialogResize<CDialogReplace>
{
private:
	int m_flags;
	pfc::string8 m_text;
	pfc::string8 m_reptext;
	HWND m_hedit;
	bool m_havefound;

public:
	CDialogReplace(HWND p_hedit) : m_hedit(p_hedit), m_flags(0), m_havefound(false)
	{
	}

	void OnFinalMessage(HWND hWnd);
	void FindResult(int pos);
	CHARRANGE GetSelection();

public:
	enum { IDD = IDD_DIALOG_REPLACE };

	BEGIN_MSG_MAP(CDialogReplace)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_ID_HANDLER_EX(IDC_FINDNEXT, OnFindNext)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
		COMMAND_HANDLER_EX(IDC_EDIT_FINDWHAT, EN_CHANGE, OnEditFindWhatEnChange)
		COMMAND_HANDLER_EX(IDC_EDIT_REPLACE, EN_CHANGE, OnEditReplaceEnChange)
		COMMAND_RANGE_HANDLER_EX(IDC_CHECK_MATCHCASE, IDC_CHECK_REGEXP, OnFlagCommand)
		COMMAND_ID_HANDLER_EX(IDC_REPLACE, OnReplace)
		COMMAND_ID_HANDLER_EX(IDC_REPLACEALL, OnReplaceall)
		CHAIN_MSG_MAP(CDialogResize<CDialogReplace>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CDialogReplace)
		DLGRESIZE_CONTROL(IDC_EDIT_FINDWHAT, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_EDIT_REPLACE, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_FINDNEXT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_REPLACE, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_REPLACEALL, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()

public:
	LRESULT OnFindUp(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnFindNext(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnEditFindWhatEnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnFlagCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnEditReplaceEnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnReplace(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnReplaceall(WORD wNotifyCode, WORD wID, HWND hWndCtl);
};
