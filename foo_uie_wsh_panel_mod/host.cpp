#include "stdafx.h"
#include "host.h"
#include "resource.h"
#include "helpers.h"
#include "panel_manager.h"
#include "global_cfg.h"
#include "popup_msg.h"
#include "dbgtrace.h"
#include "obsolete.h"


HostComm::HostComm() 
	: m_hwnd(NULL)
	, m_hdc(NULL)
	, m_width(0)
	, m_height(0)
	, m_gr_bmp(NULL)
	, m_suppress_drawing(false)
	, m_paint_pending(false)
	, m_accuracy(0) 
	, m_instance_type(KInstanceTypeCUI)
	, m_dlg_code(0)
	, m_script_info(get_config_guid())
{
	m_max_size.x = INT_MAX;
	m_max_size.y = INT_MAX;

	m_min_size.x = 0;
	m_min_size.y = 0;
}

HostComm::~HostComm()
{
}

void HostComm::Redraw()
{
	m_paint_pending = false;
	RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void HostComm::Repaint(bool force /*= false*/)
{
	m_paint_pending = true;

	if (force)
	{
		RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
	else
	{
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}

void HostComm::RepaintRect(UINT x, UINT y, UINT w, UINT h, bool force /*= false*/)
{
	RECT rc = {x, y, x + w, y + h};

	m_paint_pending = true;

	if (force)
	{
		RedrawWindow(m_hwnd, &rc, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
	else
	{
		InvalidateRect(m_hwnd, &rc, FALSE);
	}
}

void HostComm::RefreshBackground(LPRECT lprcUpdate /*= NULL*/)
{
	HWND wnd_parent = GetAncestor(m_hwnd, GA_PARENT);

	if (!wnd_parent || IsIconic(core_api::get_main_window()) || !IsWindowVisible(m_hwnd))
		return;

	HDC dc_parent = GetDC(wnd_parent);
	HDC hdc_bk = CreateCompatibleDC(dc_parent);
	POINT pt = {0, 0};
	RECT rect_child = {0, 0, m_width, m_height};
	RECT rect_parent;
	HRGN rgn_child = NULL;

	// HACK: for Tab control
	// Find siblings
	HWND hwnd = NULL;
	while (hwnd = FindWindowEx(wnd_parent, hwnd, NULL, NULL))
	{
		TCHAR buff[64];
		if (hwnd == m_hwnd) continue;
		GetClassName(hwnd, buff, _countof(buff));
		if (_tcsstr(buff, _T("SysTabControl32"))) 
		{
			wnd_parent = hwnd;
			break;
		}
	}

	if (lprcUpdate)
	{
		HRGN rgn = CreateRectRgnIndirect(lprcUpdate);
		rgn_child = CreateRectRgnIndirect(&rect_child);
		CombineRgn(rgn_child, rgn_child, rgn, RGN_DIFF);
		DeleteRgn(rgn);
	}
	else
	{
		rgn_child = CreateRectRgn(0, 0, 0, 0);
	}

	ClientToScreen(m_hwnd, &pt);
	ScreenToClient(wnd_parent, &pt);

	CopyRect(&rect_parent, &rect_child);
	ClientToScreen(m_hwnd, (LPPOINT)&rect_parent);
	ClientToScreen(m_hwnd, (LPPOINT)&rect_parent + 1);
	ScreenToClient(wnd_parent, (LPPOINT)&rect_parent);
	ScreenToClient(wnd_parent, (LPPOINT)&rect_parent + 1);

	// Force Repaint
	m_suppress_drawing = true;
	SetWindowRgn(m_hwnd, rgn_child, FALSE);
	RedrawWindow(wnd_parent, &rect_parent, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW | RDW_UPDATENOW);

	// Background bitmap
	HBITMAP old_bmp = SelectBitmap(hdc_bk, m_gr_bmp_bk);

	// Paint BK
	BitBlt(hdc_bk, rect_child.left, rect_child.top, rect_child.right - rect_child.left, rect_child.bottom - rect_child.top, 
		dc_parent, pt.x, pt.y, SRCCOPY);

	SelectBitmap(hdc_bk, old_bmp);
	DeleteDC(hdc_bk);
	ReleaseDC(wnd_parent, dc_parent);
	DeleteRgn(rgn_child);
	SetWindowRgn(m_hwnd, NULL, FALSE);
	m_suppress_drawing = false;
	SendMessage(m_hwnd, UWM_REFRESHBKDONE, 0, 0);
	if (get_edge_style()) SendMessage(m_hwnd, WM_NCPAINT, 1, 0);
	Repaint(true);
}

ITimerObj * HostComm::CreateTimerTimeout(UINT timeout)
{
	UINT id = m_host_timer_dispatcher.setTimeoutLegacy(timeout);
	if (id == 0) return NULL;
	ITimerObj * timer = new com_object_impl_t<TimerObj>(id);
	return timer;
}

ITimerObj * HostComm::CreateTimerInterval(UINT delay)
{
	UINT id = m_host_timer_dispatcher.setIntervalLegacy(delay);
	if (id == 0) return NULL;
	ITimerObj * timer = new com_object_impl_t<TimerObj>(id);
	return timer;
}

void HostComm::KillTimer(ITimerObj * p)
{
	UINT id;
	p->get_ID(&id);
	m_host_timer_dispatcher.killLegacy(id); 
}

unsigned HostComm::SetTimeout(IDispatch * func, INT delay)
{
    return m_host_timer_dispatcher.setTimeout(delay, func);
}

unsigned HostComm::SetInterval(IDispatch * func, INT delay)
{
    return m_host_timer_dispatcher.setInterval(delay, func);
}

void HostComm::ClearIntervalOrTimeout(UINT timerId)
{
    m_host_timer_dispatcher.kill(timerId);
}

IGdiBitmap * HostComm::GetBackgroundImage()
{
	Gdiplus::Bitmap * bitmap = NULL;
	IGdiBitmap * ret = NULL;

	if (get_pseudo_transparent())
	{
		bitmap = Gdiplus::Bitmap::FromHBITMAP(m_gr_bmp_bk, NULL);

		if (!helpers::ensure_gdiplus_object(bitmap))
		{
			if (bitmap) delete bitmap;
			bitmap = NULL;
		}
	}

	if (bitmap) ret = new com_object_impl_t<GdiBitmap>(bitmap);

	return ret;
}

void CALLBACK HostComm::g_timer_proc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	HWND wnd = reinterpret_cast<HWND>(dwUser);

	SendMessage(wnd, UWM_TIMER, uTimerID, 0);
}


STDMETHODIMP FbWindow::get_ID(UINT* p)
{
	TRACK_FUNCTION();

	if (!p ) return E_POINTER;

	*p = (UINT)m_host->GetHWND();
	return S_OK;
}

STDMETHODIMP FbWindow::get_Width(INT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->GetWidth();
	return S_OK;
}

STDMETHODIMP FbWindow::get_Height(INT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->GetHeight();
	return S_OK;
}

STDMETHODIMP FbWindow::get_InstanceType(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;
	*p = m_host->GetInstanceType();
	return S_OK;
}

STDMETHODIMP FbWindow::get_MaxWidth(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->GetMaxSize().x;
	return S_OK;
}

STDMETHODIMP FbWindow::put_MaxWidth(UINT width)
{
	TRACK_FUNCTION();

	m_host->GetMaxSize().x = width;
	PostMessage(m_host->GetHWND(), UWM_SIZELIMITECHANGED, 0, uie::size_limit_maximum_width);
	return S_OK;
}

STDMETHODIMP FbWindow::get_MaxHeight(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->GetMaxSize().y;
	return S_OK;
}

STDMETHODIMP FbWindow::put_MaxHeight(UINT height)
{
	TRACK_FUNCTION();

	m_host->GetMaxSize().y = height;
	PostMessage(m_host->GetHWND(), UWM_SIZELIMITECHANGED, 0, uie::size_limit_maximum_height);
	return S_OK;
}

STDMETHODIMP FbWindow::get_MinWidth(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->GetMinSize().x;
	return S_OK;
}

STDMETHODIMP FbWindow::put_MinWidth(UINT width)
{
	TRACK_FUNCTION();

	m_host->GetMinSize().x = width;
	PostMessage(m_host->GetHWND(), UWM_SIZELIMITECHANGED, 0, uie::size_limit_minimum_width);
	return S_OK;
}

STDMETHODIMP FbWindow::get_MinHeight(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->GetMinSize().y;
	return S_OK;
}

STDMETHODIMP FbWindow::put_MinHeight(UINT height)
{
	TRACK_FUNCTION();

	m_host->GetMinSize().y = height;
	PostMessage(m_host->GetHWND(), UWM_SIZELIMITECHANGED, 0, uie::size_limit_minimum_height);
	return S_OK;
}

STDMETHODIMP FbWindow::get_DlgCode(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->GetDlgCode();
	return S_OK;
}

STDMETHODIMP FbWindow::put_DlgCode(UINT code)
{
	TRACK_FUNCTION();

	m_host->GetDlgCode() = code;
	return S_OK;
}

STDMETHODIMP FbWindow::get_IsTransparent(VARIANT_BOOL* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(m_host->get_pseudo_transparent());
	return S_OK;
}

STDMETHODIMP FbWindow::get_IsVisible(VARIANT_BOOL* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = TO_VARIANT_BOOL(IsWindowVisible(m_host->GetHWND()));
	return S_OK;
}

STDMETHODIMP FbWindow::Repaint(VARIANT_BOOL force)
{
	TRACK_FUNCTION();

	m_host->Repaint(force != FALSE);
	return S_OK;
}

STDMETHODIMP FbWindow::RepaintRect(UINT x, UINT y, UINT w, UINT h, VARIANT_BOOL force)
{
	TRACK_FUNCTION();

	m_host->RepaintRect(x, y, w, h, force != FALSE);
	return S_OK;
}

STDMETHODIMP FbWindow::CreatePopupMenu(IMenuObj ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	(*pp) = new com_object_impl_t<MenuObj>(m_host->GetHWND());
	return S_OK;
}

STDMETHODIMP FbWindow::CreateTimerTimeout(UINT timeout, ITimerObj ** pp)
{
	TRACK_FUNCTION();
	if (!pp) return E_POINTER;
    print_obsolete_message("window.CreateTimerTimeout() is now obsolete, please use window.SetTimeout() in new script.");
	(*pp) = m_host->CreateTimerTimeout(timeout);
	return S_OK;
}

STDMETHODIMP FbWindow::CreateTimerInterval(UINT delay, ITimerObj ** pp)
{
	TRACK_FUNCTION();
	if (!pp) return E_POINTER;
    print_obsolete_message("window.CreateTimerInterval() is now obsolete, please use window.SetInterval() in new script.");
	(*pp)= m_host->CreateTimerInterval(delay);
	return S_OK;
}

STDMETHODIMP FbWindow::KillTimer(ITimerObj * p)
{
	TRACK_FUNCTION();
	if (!p) return E_INVALIDARG;
	m_host->KillTimer(p);
	return S_OK;
}

STDMETHODIMP FbWindow::SetInterval(IDispatch * func, INT delay, UINT * outIntervalID)
{
    TRACK_FUNCTION();
    if (!outIntervalID) return E_POINTER;
    (*outIntervalID) = m_host->SetInterval(func, delay);
    return S_OK;
}

STDMETHODIMP FbWindow::ClearInterval(UINT intervalID)
{
    TRACK_FUNCTION();
    m_host->ClearIntervalOrTimeout(intervalID);
    return S_OK;
}

STDMETHODIMP FbWindow::SetTimeout(IDispatch * func, INT delay, UINT * outTimeoutID)
{
    TRACK_FUNCTION();
    (*outTimeoutID) = m_host->SetTimeout(func, delay);
    return S_OK;
}

STDMETHODIMP FbWindow::ClearTimeout(UINT timeoutID)
{
    TRACK_FUNCTION();
    m_host->ClearIntervalOrTimeout(timeoutID);
    return S_OK;
}

STDMETHODIMP FbWindow::NotifyOthers(BSTR name, VARIANT info)
{
	TRACK_FUNCTION();

	if (!name) return E_INVALIDARG;
	if (info.vt & VT_BYREF) return E_INVALIDARG;

	HRESULT hr = S_OK;
	_variant_t var;

	hr = VariantCopy(&var, &info);

	if (FAILED(hr)) return hr;

	simple_callback_data_2<_bstr_t, _variant_t> * notify_data 
		= new simple_callback_data_2<_bstr_t, _variant_t>(name, NULL);

	notify_data->m_item2.Attach(var.Detach());

	panel_manager::instance().send_msg_to_others_pointer(m_host->GetHWND(), 
		CALLBACK_UWM_NOTIFY_DATA, notify_data);

	return S_OK;
}

STDMETHODIMP FbWindow::WatchMetadb(IFbMetadbHandle * handle)
{
	TRACK_FUNCTION();

	if (m_host->GetScriptInfo().feature_mask & t_script_info::kFeatureMetadbHandleList0)
	{
		return E_NOTIMPL;
	}

	if (!handle) return E_INVALIDARG;
	metadb_handle * ptr = NULL;
	handle->get__ptr((void**)&ptr);
	if (!ptr) return E_INVALIDARG;
	m_host->GetWatchedMetadbHandle() = ptr;
	return S_OK;
}

STDMETHODIMP FbWindow::UnWatchMetadb()
{
	TRACK_FUNCTION();

	if (m_host->GetScriptInfo().feature_mask & t_script_info::kFeatureMetadbHandleList0)
	{
		return E_NOTIMPL;
	}

	m_host->GetWatchedMetadbHandle() = NULL;
	return S_OK;
}

STDMETHODIMP FbWindow::CreateTooltip(IFbTooltip ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
    (*pp) = new com_object_impl_t<FbTooltip>(m_host->GetHWND());
	return S_OK;
}

STDMETHODIMP FbWindow::ShowConfigure()
{
	TRACK_FUNCTION();

	PostMessage(m_host->GetHWND(), UWM_SHOWCONFIGURE, 0, 0);
	return S_OK;
}

STDMETHODIMP FbWindow::ShowProperties()
{
	TRACK_FUNCTION();

	PostMessage(m_host->GetHWND(), UWM_SHOWPROPERTIES, 0, 0);
	return S_OK;
}


STDMETHODIMP FbWindow::GetProperty(BSTR name, VARIANT defaultval, VARIANT * p)
{
	TRACK_FUNCTION();

	if (!name) return E_INVALIDARG;
	if (!p) return E_POINTER;

	HRESULT hr;
	_variant_t var;
	pfc::stringcvt::string_utf8_from_wide uname(name);

	if (m_host->get_config_prop().get_config_item(uname, var))
	{
		hr = VariantCopy(p, &var);
	}
	else
	{
		m_host->get_config_prop().set_config_item(uname, defaultval);
		hr = VariantCopy(p, &defaultval);
	}

	if (FAILED(hr))
		p = NULL;

	return S_OK;
}

STDMETHODIMP FbWindow::SetProperty(BSTR name, VARIANT val)
{
	TRACK_FUNCTION();

	if (!name) return E_INVALIDARG;

	m_host->get_config_prop().set_config_item(pfc::stringcvt::string_utf8_from_wide(name), val);
	return S_OK;
}

STDMETHODIMP FbWindow::GetBackgroundImage(IGdiBitmap ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

	(*pp) = m_host->GetBackgroundImage();
	return S_OK;
}

STDMETHODIMP FbWindow::SetCursor(UINT id)
{
	TRACK_FUNCTION();

	::SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(id)));
	return S_OK;
}

STDMETHODIMP FbWindow::GetColorCUI(UINT type, BSTR guidstr, DWORD * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;
	if (!guidstr) return E_INVALIDARG;
	if (m_host->GetInstanceType() != HostComm::KInstanceTypeCUI) return E_NOTIMPL;

	GUID guid;

	if (!*guidstr)
	{
		memcpy(&guid, &pfc::guid_null, sizeof(guid));
	}
	else
	{
		if (CLSIDFromString(guidstr, &guid) != NOERROR)
		{
			return E_INVALIDARG;
		}
	}

	*p = m_host->GetColorCUI(type, guid);
	return S_OK;
}

STDMETHODIMP FbWindow::GetFontCUI(UINT type, BSTR guidstr, IGdiFont ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	if (!guidstr) return E_INVALIDARG;
	if (m_host->GetInstanceType() != HostComm::KInstanceTypeCUI) return E_NOTIMPL;

	GUID guid;

	if (!*guidstr)
	{
		memcpy(&guid, &pfc::guid_null, sizeof(guid));
	}
	else
	{
		if (CLSIDFromString(guidstr, &guid) != NOERROR)
		{
			return E_INVALIDARG;
		}
	}

	HFONT hFont = m_host->GetFontCUI(type, guid);

	*pp = NULL;

	if (hFont)
	{
		Gdiplus::Font * font = new Gdiplus::Font(m_host->GetHDC(), hFont);

		if (!helpers::ensure_gdiplus_object(font))
		{
			if (font) delete font;
			(*pp) = NULL;
			return S_OK;
		}

		*pp = new com_object_impl_t<GdiFont>(font, hFont);
	}

	return S_OK;
}

STDMETHODIMP FbWindow::GetColorDUI(UINT type, DWORD * p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;
	if (m_host->GetInstanceType() != HostComm::KInstanceTypeDUI) return E_NOTIMPL;

	*p = m_host->GetColorDUI(type);
	return S_OK;
}

STDMETHODIMP FbWindow::GetFontDUI(UINT type, IGdiFont ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;
	if (m_host->GetInstanceType() != HostComm::KInstanceTypeDUI) return E_NOTIMPL;

	HFONT hFont = m_host->GetFontDUI(type);
	*pp = NULL;

	if (hFont)
	{
		Gdiplus::Font * font = new Gdiplus::Font(m_host->GetHDC(), hFont);

		if (!helpers::ensure_gdiplus_object(font))
		{
			if (font) delete font;
			(*pp) = NULL;
			return S_OK;
		}

		*pp = new com_object_impl_t<GdiFont>(font, hFont, false);
	}

	return S_OK;
}

STDMETHODIMP FbWindow::CreateThemeManager(BSTR classid, IThemeManager ** pp)
{
	TRACK_FUNCTION();

	if (!classid) return E_INVALIDARG;
	if (!pp) return E_POINTER;

	IThemeManager * ptheme = NULL;

	try
	{
		ptheme = new com_object_impl_t<ThemeManager>(m_host->GetHWND(), classid);
	}
	catch (pfc::exception_invalid_params &)
	{
		if (ptheme)
		{
			ptheme->Dispose();
			delete ptheme;
			ptheme = NULL;
		}
	}

	*pp = ptheme;
	return S_OK;
}


ScriptHost::ScriptHost(HostComm * host) 
	: m_host(host)
	, m_window(new com_object_impl_t<FbWindow, false>(host))
	, m_gdi(com_object_singleton_t<GdiUtils>::instance())
	, m_fb2k(com_object_singleton_t<FbUtils>::instance())
	, m_utils(com_object_singleton_t<WSHUtils>::instance())
    , m_playlistman(com_object_singleton_t<FbPlaylistManager>::instance())
	, m_dwStartTime(0)
	, m_dwRef(1)
	, m_engine_inited(false)
	, m_has_error(false)
    , m_lastSourceContext(0)
{
	if (g_cfg_debug_mode)
	{
		HRESULT hr = S_OK;

		if (SUCCEEDED(hr)) hr = m_debug_manager.CreateInstance(CLSID_ProcessDebugManager, NULL, CLSCTX_INPROC_SERVER);
		if (SUCCEEDED(hr)) hr = m_debug_manager->CreateApplication(&m_debug_application);
        if (SUCCEEDED(hr)) hr = m_debug_application->SetName( _T(WSPM_NAME) );
		if (SUCCEEDED(hr)) hr = m_debug_manager->AddApplication(m_debug_application, &m_app_cookie);

		if (FAILED(hr))
		{
			g_cfg_debug_mode = false;

			popup_msg::g_show("Debug mod is enabled but it seems that there is no valid script debugger "
				"associated with your system, so this option will be disabled.", WSPM_NAME);
		}
	}
}

ScriptHost::~ScriptHost()
{
	if (m_debug_manager)
	{
		m_debug_manager->RemoveApplication(m_app_cookie);
		m_debug_manager.Release();
	}

	if (m_debug_application)
	{
		m_debug_application->Close();
		m_debug_application.Release();
	}
}

STDMETHODIMP_(ULONG) ScriptHost::AddRef()
{
    return InterlockedIncrement(&m_dwRef);
}

STDMETHODIMP_(ULONG) ScriptHost::Release()
{
    ULONG n = InterlockedDecrement(&m_dwRef); 

    if (n == 0)
    {
        delete this;
    }

    return n;
}

STDMETHODIMP ScriptHost::GetLCID(LCID* plcid)
{
	return E_NOTIMPL;
}

STDMETHODIMP ScriptHost::GetItemInfo(LPCOLESTR name, DWORD mask, IUnknown** ppunk, ITypeInfo** ppti)
{
	if (ppti) *ppti = NULL;

	if (ppunk) *ppunk = NULL;

	if (mask & SCRIPTINFO_IUNKNOWN)
	{
		if (!name) return E_INVALIDARG;
		if (!ppunk) return E_POINTER;

		if (wcscmp(name, L"window") == 0)
		{
			(*ppunk) = m_window;
			(*ppunk)->AddRef();
			return S_OK;
		}
		else if (wcscmp(name, L"gdi") == 0)
		{
			(*ppunk) = m_gdi;
			(*ppunk)->AddRef();
			return S_OK;
		}
		else if (wcscmp(name, L"fb") == 0)
		{
			(*ppunk) = m_fb2k;
			(*ppunk)->AddRef();
			return S_OK;
		}
		else if (wcscmp(name, L"utils") == 0)
		{
			(*ppunk) = m_utils;
			(*ppunk)->AddRef();
			return S_OK;
		}
        else if (wcscmp(name, L"plman") == 0)
        {
            (*ppunk) = m_playlistman;
            (*ppunk)->AddRef();
            return S_OK;
        }
	}

	return TYPE_E_ELEMENTNOTFOUND;
}

STDMETHODIMP ScriptHost::GetDocVersionString(BSTR* pstr)
{
	return E_NOTIMPL;
}

STDMETHODIMP ScriptHost::OnScriptTerminate(const VARIANT* result, const EXCEPINFO* excep)
{
	return E_NOTIMPL;
}

STDMETHODIMP ScriptHost::OnStateChange(SCRIPTSTATE state)
{
	return E_NOTIMPL;
}

STDMETHODIMP ScriptHost::OnScriptError(IActiveScriptError* err)
{
	m_has_error = true;

	if (!err) return E_POINTER;

	ReportError(err);
	return S_OK;
}

STDMETHODIMP ScriptHost::OnEnterScript()
{
	m_dwStartTime = GetTickCount();
	return S_OK;
}

STDMETHODIMP ScriptHost::OnLeaveScript()
{
	return S_OK;
}

STDMETHODIMP ScriptHost::GetDocumentContextFromPosition(DWORD dwSourceContext, ULONG uCharacterOffset, ULONG uNumChars, IDebugDocumentContext **ppsc)
{
	if (g_cfg_debug_mode)
	{
		t_debug_doc_map::iterator iter = m_debug_docs.find(dwSourceContext);

		if (iter.is_valid())
		{
			IDebugDocumentHelper * helper = iter->m_value;
			ULONG ulStartPos = 0;
			HRESULT hr = helper->GetScriptBlockInfo(dwSourceContext, NULL, &ulStartPos, NULL);

			if (SUCCEEDED(hr)) 
			{
				hr = helper->CreateDebugDocumentContext(ulStartPos + uCharacterOffset, uNumChars, ppsc);
			}

			return hr;
		}
	}

	return E_FAIL;
}

STDMETHODIMP ScriptHost::GetApplication(IDebugApplication **ppda)
{
	if (!ppda) return E_POINTER;

	if (m_debug_application)
	{
		// FIXME: Should AddRef() first?
		m_debug_application.AddRef();
		*ppda = m_debug_application;
	}
	else
	{
		return E_NOTIMPL;
	}

	return S_OK;
}

STDMETHODIMP ScriptHost::GetRootApplicationNode(IDebugApplicationNode **ppdanRoot)
{
	if (!ppdanRoot) return E_POINTER;

	if (m_debug_application)
		return m_debug_application->GetRootNode(ppdanRoot);
	else
		*ppdanRoot = NULL;

	return S_OK;
}

STDMETHODIMP ScriptHost::OnScriptErrorDebug(IActiveScriptErrorDebug *pErrorDebug, BOOL *pfEnterDebugger, BOOL *pfCallOnScriptErrorWhenContinuing)
{
	m_has_error = true;

	if (!pErrorDebug || !pfEnterDebugger || !pfCallOnScriptErrorWhenContinuing)
		return E_POINTER;

	int ret = ::uMessageBox(core_api::get_main_window(), 
		"A script error occured. Do you want to debug?",
		WSPM_NAME,
		MB_ICONQUESTION | MB_SETFOREGROUND | MB_YESNO);

	*pfEnterDebugger = (ret == IDYES);
	*pfCallOnScriptErrorWhenContinuing = FALSE;

	if (ret == IDNO)
		ReportError(pErrorDebug);
	else
		SendMessage(m_host->GetHWND(), UWM_SCRIPT_ERROR, 0, 0);

	return S_OK;
}

STDMETHODIMP ScriptHost::GetWindow(HWND *phwnd)
{
	*phwnd = m_host->GetHWND();

	return S_OK;
}

STDMETHODIMP ScriptHost::EnableModeless(BOOL fEnable)
{
	return S_OK;
}

HRESULT ScriptHost::Initialize()
{
	Finalize();

	m_has_error = false;

	HRESULT hr = S_OK;
	IActiveScriptParsePtr parser;
	pfc::stringcvt::string_wide_from_utf8_fast wname(m_host->get_script_engine());
	pfc::stringcvt::string_wide_from_utf8_fast wcode(m_host->get_script_code());
	// Load preprocessor module
	script_preprocessor preprocessor(wcode.get_ptr());
	preprocessor.process_script_info(m_host->GetScriptInfo());

    hr = InitScriptEngineByName(wname);

	if (SUCCEEDED(hr)) hr = m_script_engine->SetScriptSite(this);
	if (SUCCEEDED(hr)) hr = m_script_engine->QueryInterface(&parser);
	if (SUCCEEDED(hr)) hr = parser->InitNew();

    EnableSafeModeToScriptEngine(g_cfg_safe_mode);

	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"window", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"gdi", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"fb", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"utils", SCRIPTITEM_ISVISIBLE);
    if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"plman", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->SetScriptState(SCRIPTSTATE_CONNECTED);
	if (SUCCEEDED(hr)) hr = m_script_engine->GetScriptDispatch(NULL, &m_script_root);
    // Parse imported scripts
    if (SUCCEEDED(hr)) hr = ProcessImportedScripts(preprocessor, parser);

	// Parse main script
	DWORD source_context = 0;
	if (SUCCEEDED(hr)) hr = GenerateSourceContext(NULL, wcode, source_context);
    m_contextToPathMap[source_context] = "<main>";

	if (SUCCEEDED(hr)) hr = parser->ParseScriptText(wcode.get_ptr(), NULL, NULL, NULL, 
		source_context, 0, SCRIPTTEXT_HOSTMANAGESSOURCE | SCRIPTTEXT_ISVISIBLE, NULL, NULL);

	if (SUCCEEDED(hr)) m_engine_inited = true;
    m_callback_invoker.init(m_script_root);
	return hr;
}

void ScriptHost::EnableSafeModeToScriptEngine(bool enable)
{
    if (!enable)
        return;

    _COM_SMARTPTR_TYPEDEF(IObjectSafety, IID_IObjectSafety);
    IObjectSafetyPtr psafe;

    if (SUCCEEDED(m_script_engine->QueryInterface(&psafe)))
    {
        psafe->SetInterfaceSafetyOptions(IID_IDispatch, 
            INTERFACE_USES_SECURITY_MANAGER, INTERFACE_USES_SECURITY_MANAGER);
    }
}

HRESULT ScriptHost::ProcessImportedScripts(script_preprocessor &preprocessor, IActiveScriptParsePtr &parser)
{
    // processing "@import"
    script_preprocessor::t_script_list scripts;
    HRESULT hr = preprocessor.process_import(m_host->GetScriptInfo(), scripts);

    for (t_size i = 0; i < scripts.get_count(); ++i)
    {
        DWORD source_context;

        if (SUCCEEDED(hr)) hr = GenerateSourceContext(scripts[i].path.get_ptr(), scripts[i].code.get_ptr(), source_context);
        if (FAILED(hr)) break;
        m_contextToPathMap[source_context] = pfc::stringcvt::string_utf8_from_wide(scripts[i].path.get_ptr());
        hr = parser->ParseScriptText(scripts[i].code.get_ptr(), NULL, NULL, NULL, 
            source_context, 0, SCRIPTTEXT_HOSTMANAGESSOURCE | SCRIPTTEXT_ISVISIBLE, NULL, NULL);
    }

    return hr;
}

HRESULT ScriptHost::InitScriptEngineByName(const wchar_t * engineName)
{
    HRESULT hr;

    if (wcscmp(engineName, L"JScript9") == 0) 
    {
        // Try using JScript9 from IE9
        // {16d51579-a30b-4c8b-a276-0ff4dc41e755}
        static const CLSID clsid = 
        {0x16d51579, 0xa30b, 0x4c8b, {0xa2, 0x76, 0x0f, 0xf4, 0xdc, 0x41, 0xe7, 0x55 } };

        if (FAILED(hr = m_script_engine.CreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER)))
        {
            // fallback to default JScript engine.
            hr = m_script_engine.CreateInstance(L"JScript", NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER);
        }
    }
    else
    {
        hr = m_script_engine.CreateInstance(engineName, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER);
    }

    // In order to support new features after JScript 5.8
    const wchar_t jscriptName[] = L"JScript";
    if (wcsncmp(engineName, jscriptName, _countof(jscriptName) - 1) == 0)
    {
        IActiveScriptProperty *pActScriProp = NULL;
        
        if (SUCCEEDED(m_script_engine->QueryInterface(IID_IActiveScriptProperty, (void **)&pActScriProp)))
        {
            VARIANT scriptLangVersion;
            scriptLangVersion.vt = VT_I4;
            scriptLangVersion.lVal = SCRIPTLANGUAGEVERSION_5_8;
            if (FAILED(pActScriProp->SetProperty(SCRIPTPROP_INVOKEVERSIONING, NULL, &scriptLangVersion)))
            {
                // Reset to default
                scriptLangVersion.lVal = SCRIPTLANGUAGEVERSION_DEFAULT;
                pActScriProp->SetProperty(SCRIPTPROP_INVOKEVERSIONING, NULL, &scriptLangVersion);
            }
            pActScriProp->Release();
        }
    }

    return hr;
}

void ScriptHost::Finalize()
{
	InvokeCallback(CallbackIds::on_script_unload);

	if (m_script_engine && m_engine_inited)
	{
		// Call GC explicitly 
 		IActiveScriptGarbageCollector * gc = NULL;
 		if (SUCCEEDED(m_script_engine->QueryInterface(IID_IActiveScriptGarbageCollector, (void **)&gc)))
 		{
 			gc->CollectGarbage(SCRIPTGCTYPE_EXHAUSTIVE);
 			gc->Release();
 		}

		m_script_engine->SetScriptState(SCRIPTSTATE_DISCONNECTED);
		m_script_engine->InterruptScriptThread(SCRIPTTHREADID_ALL, NULL, 0);
		m_engine_inited = false;
	}

	for (t_debug_doc_map::iterator iter = m_debug_docs.first(); iter.is_valid(); ++iter)
	{
		IDebugDocumentHelperPtr & helper = iter->m_value;

		if (helper)
		{
			helper->Detach();
			helper.Release();
		}
	}

	m_debug_docs.remove_all();
    m_contextToPathMap.remove_all();
    m_callback_invoker.reset();

	if (m_script_engine)
	{
		m_script_engine->Close();
		m_script_engine.Release();
	}

	if (m_script_root)
	{
		m_script_root.Release();
	}
}

HRESULT ScriptHost::InvokeCallback(int callbackId, VARIANTARG * argv /*= NULL*/, UINT argc /*= 0*/, VARIANT * ret /*= NULL*/)
{
	if (HasError()) return E_FAIL;
	if (!m_script_root || !Ready()) return E_FAIL;
	
    HRESULT hr = E_FAIL;

	try
	{
		hr = m_callback_invoker.invoke(callbackId, argv, argc, ret);
	}
	catch (std::exception & e)
	{
		pfc::print_guid guid(m_host->get_config_guid());
		console::printf(WSPM_NAME " (%s): Unhandled C++ Exception: \"%s\", will crash now...", 
			m_host->GetScriptInfo().build_info_string().get_ptr(), e.what());
		PRINT_DISPATCH_TRACK_MESSAGE_AND_BREAK();
	}
    catch (_com_error & e)
    {
        pfc::print_guid guid(m_host->get_config_guid());
        console::printf(WSPM_NAME " (%s): Unhandled COM Error: \"%s\", will crash now...", 
            m_host->GetScriptInfo().build_info_string().get_ptr(), 
            pfc::stringcvt::string_utf8_from_wide(e.ErrorMessage()).get_ptr());
        PRINT_DISPATCH_TRACK_MESSAGE_AND_BREAK();
    }
	catch (...)
	{
		pfc::print_guid guid(m_host->get_config_guid());
		console::printf(WSPM_NAME " (%s): Unhandled Unknown Exception, will crash now...", 
			m_host->GetScriptInfo().build_info_string().get_ptr());
        PRINT_DISPATCH_TRACK_MESSAGE_AND_BREAK();
	}

	return hr;
}

HRESULT ScriptHost::GenerateSourceContext(const wchar_t * path, const wchar_t * code, DWORD & source_context)
{
	pfc::stringcvt::string_wide_from_utf8_fast name, guidString;
	HRESULT hr = S_OK;
	t_size len = wcslen(code);

	if (!path) 
	{
		if (m_host->GetScriptInfo().name.is_empty())
			name.convert(pfc::print_guid(m_host->GetGUID()));
		else
			name.convert(m_host->GetScriptInfo().name);

		guidString.convert(pfc::print_guid(m_host->GetGUID()));
	}	

	if (m_debug_manager) 
	{
		IDebugDocumentHelperPtr docHelper;

		if (SUCCEEDED(hr)) hr = m_debug_manager->CreateDebugDocumentHelper(NULL, &docHelper);
		if (SUCCEEDED(hr)) hr = docHelper->Init(m_debug_application, 
			(!path) ? name.get_ptr() : path, 
			(!path) ? guidString.get_ptr() : path,
			TEXT_DOC_ATTR_READONLY);
		if (SUCCEEDED(hr)) hr = docHelper->Attach(NULL);
		if (SUCCEEDED(hr)) hr = docHelper->AddUnicodeText(code);
		if (SUCCEEDED(hr)) hr = docHelper->DefineScriptBlock(0, len, m_script_engine, FALSE, &source_context);
		if (SUCCEEDED(hr)) m_debug_docs.find_or_add(source_context) = docHelper;
	}
	else
	{
		source_context = m_lastSourceContext++;
	}

	return hr;
}

void ScriptHost::ReportError(IActiveScriptError* err)
{
	if (!err) return;

	DWORD ctx = 0;
	ULONG line = 0;
	LONG  charpos = 0;
	EXCEPINFO excep = { 0 };
	//WCHAR buf[512] = { 0 };
	_bstr_t sourceline;
	_bstr_t name;

	if (FAILED(err->GetSourcePosition(&ctx, &line, &charpos)))
	{
		line = 0;
		charpos = 0;
	}

	if (FAILED(err->GetSourceLineText(sourceline.GetAddress())))
	{
		sourceline = L"<source text only available at compile time>";
	}

	// HACK: Try to retrieve additional infomation from Debug Manger Interface
    CallDebugManager(err, name, line, sourceline);

	if (FAILED(err->GetExceptionInfo(&excep)))
        return;

    // Do a deferred fill-in if necessary
    if (excep.pfnDeferredFillIn)
        (*excep.pfnDeferredFillIn)(&excep);

    using namespace pfc::stringcvt;
    pfc::string_formatter formatter;
    formatter << WSPM_NAME << " (" << m_host->GetScriptInfo().build_info_string().get_ptr() << "): ";

    if (excep.bstrSource && excep.bstrDescription) 
    {
        formatter << string_utf8_from_wide(excep.bstrSource) << ":\n";
        formatter << string_utf8_from_wide(excep.bstrDescription) << "\n";
    }
    else
    {
        pfc::string8_fast errorMessage;

        if (uFormatSystemErrorMessage(errorMessage, excep.scode))
            formatter << errorMessage;
        else
            formatter << "Unknown error code: 0x" << pfc::format_hex_lowercase((unsigned)excep.scode);
    }

    if (m_contextToPathMap.exists(ctx))
    {
        formatter << "File: " << m_contextToPathMap[ctx] << "\n";
    }

    formatter << "Ln: " << (t_uint32)(line + 1) << ", Col: " << (t_uint32)(charpos + 1) << "\n";
    formatter << string_utf8_from_wide(sourceline);
    if (name.length() > 0) formatter << "\nAt: " << name;

    SendMessage(m_host->GetHWND(), UWM_SCRIPT_ERROR, 0, (LPARAM)formatter.get_ptr());

    if (excep.bstrSource)      SysFreeString(excep.bstrSource);
    if (excep.bstrDescription) SysFreeString(excep.bstrDescription);
    if (excep.bstrHelpFile)    SysFreeString(excep.bstrHelpFile);
}

void ScriptHost::CallDebugManager(IActiveScriptError* err, _bstr_t &name, ULONG line, _bstr_t &sourceline)
{
    if (m_debug_manager)
    {
        _COM_SMARTPTR_TYPEDEF(IActiveScriptErrorDebug, IID_IActiveScriptErrorDebug);
        _COM_SMARTPTR_TYPEDEF(IDebugDocumentContext, IID_IDebugDocumentContext);
        _COM_SMARTPTR_TYPEDEF(IDebugDocumentText, IID_IDebugDocumentText);
        _COM_SMARTPTR_TYPEDEF(IDebugDocument, IID_IDebugDocument);

        HRESULT hr;
        IActiveScriptErrorDebugPtr  pErrorDebug;
        IDebugDocumentContextPtr    context;
        IDebugDocumentTextPtr       text;
        IDebugDocumentPtr           document;

        hr = err->QueryInterface(IID_IActiveScriptErrorDebug, (void **)&pErrorDebug);

        if (SUCCEEDED(hr)) hr = pErrorDebug ? pErrorDebug->GetDocumentContext(&context) : E_FAIL;
        if (SUCCEEDED(hr)) hr = context ? context->GetDocument(&document) : E_FAIL;
        if (SUCCEEDED(hr)) hr = document ? document->QueryInterface(IID_IDebugDocumentText, (void **)&text) : E_FAIL;

        if (SUCCEEDED(hr))
        {
            ULONG charPosLine = 0;
            ULONG linePos = 0;
            ULONG lines = 0;
            ULONG numChars = 0;
            ULONG charsToRead;
            // Must be set to 0 first
            ULONG charsRead = 0;

            text->GetName(DOCUMENTNAMETYPE_TITLE, name.GetAddress());
            text->GetPositionOfLine(line, &charPosLine);
            text->GetSize(&lines, &numChars);

            if (lines > line)
            {
                // Has next line?
                ULONG charPosNextLine;

                text->GetPositionOfLine(line + 1, &charPosNextLine);
                charsToRead = charPosNextLine - 1 - charPosLine;
            }
            else
            {
                // Read to the end of current line.
                charsToRead = numChars - charPosLine;
            }

            pfc::array_t<wchar_t> code;
            code.set_size(charsToRead + 1);
            text->GetText(charPosLine, code.get_ptr(), NULL, &charsRead, charsToRead);
            code[charsRead] = 0;
            --charsRead;

            while (charsRead && (code[charsRead] == '\r' || code[charsRead] == '\n'))
            {
                code[charsRead] = 0;
                --charsRead;
            }

            if (*code.get_ptr())
                sourceline = code.get_ptr();
        }
    }
}
