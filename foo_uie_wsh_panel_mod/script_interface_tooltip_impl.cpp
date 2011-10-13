#include "stdafx.h"
#include "script_interface_impl.h"
#include "script_interface_tooltip_impl.h"
#include "helpers.h"
#include "com_array.h"
#include "panel_manager.h"


FbTooltip::FbTooltip(HWND p_wndparent/*, bool p_want_multiline*/) 
    : m_wndparent(p_wndparent)
    , m_tip_buffer(SysAllocString(PFC_WIDESTRING(WSPM_NAME)))
{
    m_wndtooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        CW_USEDEFAULT, CW_USEDEFAULT,
        p_wndparent, NULL, core_api::get_my_instance(), NULL);

    // Original position
    SetWindowPos(m_wndtooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Set up tooltip information.
    TOOLINFO ti = { 0 };

    ti.cbSize = sizeof(ti);
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hinst = NULL;
    ti.hwnd = p_wndparent;
    ti.uId = (UINT_PTR)p_wndparent;
    ti.lpszText = m_tip_buffer;

    SendMessage(m_wndtooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);	
    SendMessage(m_wndtooltip, TTM_ACTIVATE, FALSE, 0);
}

void FbTooltip::FinalRelease()
{
    if (m_wndtooltip/* && IsWindow(m_wndtooltip)*/)
    {
        DestroyWindow(m_wndtooltip);
        m_wndtooltip = NULL;

        panel_store & store = panel_manager::instance().query_store_by_window(m_wndparent);
        store.tooltipCount--;
    }

    if (m_tip_buffer)
    {
        SysFreeString(m_tip_buffer);
        m_tip_buffer = NULL;
    }
}

STDMETHODIMP FbTooltip::get_Text(BSTR * pp)
{
    TRACK_FUNCTION();

    if (!pp) return E_POINTER;

    (*pp) = SysAllocString(m_tip_buffer);
    return S_OK;
}

STDMETHODIMP FbTooltip::put_Text(BSTR text)
{
    TRACK_FUNCTION();

    if (!text) return E_INVALIDARG;

    // realloc string
    SysReAllocString(&m_tip_buffer, text);

    TOOLINFO ti = { 0 };

    ti.cbSize = sizeof(ti);
    ti.hinst = NULL;
    ti.hwnd = m_wndparent;
    ti.uId = (UINT_PTR)m_wndparent;
    ti.lpszText = m_tip_buffer;
    SendMessage(m_wndtooltip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
    return S_OK;
}

STDMETHODIMP FbTooltip::Activate()
{
    TRACK_FUNCTION();

    SendMessage(m_wndtooltip, TTM_ACTIVATE, TRUE, 0);
    return S_OK;
}

STDMETHODIMP FbTooltip::Deactivate()
{
    TRACK_FUNCTION();

    SendMessage(m_wndtooltip, TTM_ACTIVATE, FALSE, 0);
    return S_OK;
}

STDMETHODIMP FbTooltip::SetMaxWidth(int width)
{
    TRACK_FUNCTION();

    SendMessage(m_wndtooltip, TTM_SETMAXTIPWIDTH, 0, width);
    return S_OK;
}

STDMETHODIMP FbTooltip::GetDelayTime(int type, INT * p)
{
    TRACK_FUNCTION();

    if (!p) return E_POINTER;
    if (type < TTDT_AUTOMATIC || type > TTDT_INITIAL) return E_INVALIDARG;

    *p = SendMessage(m_wndtooltip, TTM_GETDELAYTIME, type, 0);
    return S_OK;
}

STDMETHODIMP FbTooltip::SetDelayTime(int type, int time)
{
    TRACK_FUNCTION();

    if (type < TTDT_AUTOMATIC || type > TTDT_INITIAL) return E_INVALIDARG;

    SendMessage(m_wndtooltip, TTM_SETDELAYTIME, type, time);
    return S_OK;
}
