#include "stdafx.h"
#include "host.h"
#include "resource.h"
#include "helpers.h"
#include "panel_notifier.h"
#include "global_cfg.h"
#include "ui_conf.h"
#include "ui_property.h"
#include "popup_msg.h"
#include "dbgtrace.h"


namespace
{
	// Just because I don't want to include the helpers
	template<typename TImpl>
	class my_ui_element_impl : public ui_element 
	{
	public:
		GUID get_guid() { return TImpl::g_get_guid();}
		GUID get_subclass() { return TImpl::g_get_subclass();}
		void get_name(pfc::string_base & out) { TImpl::g_get_name(out); }

		ui_element_instance::ptr instantiate(HWND parent,ui_element_config::ptr cfg,ui_element_instance_callback::ptr callback) 
		{
			PFC_ASSERT( cfg->get_guid() == get_guid() );
			service_nnptr_t<ui_element_instance_impl_helper> item = new service_impl_t<ui_element_instance_impl_helper>(cfg, callback);
			item->initialize_window(parent);
			return item;
		}

		ui_element_config::ptr get_default_configuration() { return TImpl::g_get_default_configuration(); }
		ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) {return NULL;}
		bool get_description(pfc::string_base & out) {out = TImpl::g_get_description(); return true;}

	private:
		class ui_element_instance_impl_helper : public TImpl 
		{
		public:
			ui_element_instance_impl_helper(ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback)
				: TImpl(cfg, callback) {}
		};
	};

	// CUI panel instance
	static uie::window_factory<wsh_panel_window_cui> g_wsh_panel_wndow_cui;
	// DUI panel instance
	static service_factory_t<my_ui_element_impl<wsh_panel_window_dui> > g_wsh_panel_wndow_dui;
}

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

	TIMECAPS tc;

	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR)
	{
		m_accuracy = min(max(tc.wPeriodMin, 5), tc.wPeriodMax);
	}

	timeBeginPeriod(m_accuracy);
}

HostComm::~HostComm()
{
	timeEndPeriod(m_accuracy);
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
	UINT id = timeSetEvent(timeout, m_accuracy, g_timer_proc, reinterpret_cast<DWORD_PTR>(m_hwnd), TIME_ONESHOT);

	if (id == NULL) return NULL;

	ITimerObj * timer = new com_object_impl_t<TimerObj>(id);
	return timer;
}

ITimerObj * HostComm::CreateTimerInterval(UINT delay)
{
	UINT id = timeSetEvent(delay, m_accuracy, g_timer_proc, reinterpret_cast<DWORD_PTR>(m_hwnd), TIME_PERIODIC);

	if (id == NULL) return NULL;

	ITimerObj * timer = new com_object_impl_t<TimerObj>(id);
	return timer;
}

void HostComm::KillTimer(ITimerObj * p)
{
	UINT id;

	p->get_ID(&id);
	timeKillEvent(id); 
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

	(*pp) = m_host->CreateTimerTimeout(timeout);
	return S_OK;
}

STDMETHODIMP FbWindow::CreateTimerInterval(UINT delay, ITimerObj ** pp)
{
	TRACK_FUNCTION();

	if (!pp) return E_POINTER;

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

	panel_notifier_manager::instance().send_msg_to_others_pointer(m_host->GetHWND(), 
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

//STDMETHODIMP FbWindow::CreateObject(BSTR progid_or_clsid, IUnknown ** pp)
//{
//	TRACK_FUNCTION();
//
//	if (!progid_or_clsid) return E_INVALIDARG;
//	if (!pp) return E_POINTER;
//
//	HRESULT hr = S_OK;
//	CLSID clsid;
//	IUnknownPtr unk;
//	*pp = NULL;
//	
//	// Check safety
//	if (g_cfg_safe_mode)
//	{
//		if (progid_or_clsid[0] == '{')
//		{
//			hr = CLSIDFromString(progid_or_clsid, &clsid);
//		}
//		else 
//		{
//			hr = CLSIDFromProgID(progid_or_clsid, &clsid);
//		}
//
//
//	}
//
//	if (SUCCEEDED(hr)) hr = unk.CreateInstance(clsid, NULL, CLSCTX_ALL);
//	*pp = unk.Detach();
//	return hr;
//}

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
        else if (wcscmp(name, L"playlistman") == 0)
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

	if (SUCCEEDED(hr)) hr = m_script_engine.CreateInstance(wname.get_ptr(), NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER);
	if (SUCCEEDED(hr)) hr = m_script_engine->SetScriptSite(this);
	if (SUCCEEDED(hr)) hr = m_script_engine->QueryInterface(&parser);
	if (SUCCEEDED(hr)) hr = parser->InitNew();

	if (g_cfg_safe_mode)
	{
		_COM_SMARTPTR_TYPEDEF(IObjectSafety, IID_IObjectSafety);

		IObjectSafetyPtr psafe;

		if (SUCCEEDED(m_script_engine->QueryInterface(&psafe)))
		{
			psafe->SetInterfaceSafetyOptions(IID_IDispatch, 
				INTERFACE_USES_SECURITY_MANAGER, INTERFACE_USES_SECURITY_MANAGER);
		}
	}

	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"window", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"gdi", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"fb", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"utils", SCRIPTITEM_ISVISIBLE);
    if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"playlistman", SCRIPTITEM_ISVISIBLE);
	//if (SUCCEEDED(hr)) hr = m_script_engine->SetScriptState(SCRIPTSTATE_STARTED);
	if (SUCCEEDED(hr)) hr = m_script_engine->SetScriptState(SCRIPTSTATE_CONNECTED);
	if (SUCCEEDED(hr)) hr = m_script_engine->GetScriptDispatch(NULL, &m_script_root);
	
	// processing "@import"
	{
		script_preprocessor::t_script_list scripts;
		if (SUCCEEDED(hr)) hr = preprocessor.process_import(m_host->GetScriptInfo(), scripts);

		for (t_size i = 0; i < scripts.get_count(); ++i)
		{
			DWORD source_context;

			if (SUCCEEDED(hr)) 
				hr = GenerateSourceContext(scripts[i].path.get_ptr(), scripts[i].code.get_ptr(), source_context);

			if (SUCCEEDED(hr))
			{
				hr = parser->ParseScriptText(scripts[i].code.get_ptr(), NULL, NULL, NULL, 
					source_context, 0, SCRIPTTEXT_HOSTMANAGESSOURCE | SCRIPTTEXT_ISVISIBLE, NULL, NULL);
			}
			else
			{
				break;
			}
		}
	}

	// Parse main script
	DWORD source_context;
	if (SUCCEEDED(hr)) hr = GenerateSourceContext(NULL, wcode, source_context);
	if (SUCCEEDED(hr)) hr = parser->ParseScriptText(wcode.get_ptr(), NULL, NULL, NULL, 
		source_context, 0, SCRIPTTEXT_HOSTMANAGESSOURCE | SCRIPTTEXT_ISVISIBLE, NULL, NULL);

	//
	if (SUCCEEDED(hr)) m_engine_inited = true;
	return hr;
}

void ScriptHost::Finalize()
{
	InvokeV(L"on_script_unload");

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

HRESULT ScriptHost::InvokeV(LPOLESTR name, VARIANTARG * argv /*= NULL*/, UINT argc /*= 0*/, VARIANT * ret /*= NULL*/)
{
	if (HasError()) return E_FAIL;
	if (!m_script_root || !Ready()) return E_FAIL;
	if (!name) return E_INVALIDARG;

	DISPID dispid = 0;
	DISPPARAMS param = { argv, NULL, argc, 0 };
	IDispatchPtr disp = m_script_root;
	
	HRESULT hr = disp->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);

	if (SUCCEEDED(hr))
	{
		try
		{
			hr = disp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &param, ret, NULL, NULL);
		}
		catch (std::exception & e)
		{
			pfc::print_guid guid(m_host->get_config_guid());

			console::printf(WSPM_NAME " (%s): Unhandled C++ Exception: \"%s\", will crash now...", 
				m_host->GetScriptInfo().build_info_string().get_ptr(), e.what());
			PRINT_DISPATCH_TRACK_MESSAGE();
			// breakpoint
			__debugbreak();
		}
		catch (...)
		{
			pfc::print_guid guid(m_host->get_config_guid());

			console::printf(WSPM_NAME " (%s): Unhandled Unknown Exception, will crash now...", 
				m_host->GetScriptInfo().build_info_string().get_ptr());
			PRINT_DISPATCH_TRACK_MESSAGE();
			// breakpoint
			__debugbreak();
		}
	}

	return hr;
}

HRESULT ScriptHost::GenerateSourceContext(const wchar_t * path, const wchar_t * code, DWORD & source_context)
{
	pfc::stringcvt::string_wide_from_utf8_fast namebuf, guidstring;
	HRESULT hr = S_OK;
	t_size len = wcslen(code);

	if (!path) 
	{
		if (m_host->GetScriptInfo().name.is_empty())
			namebuf.convert(pfc::print_guid(m_host->GetGUID()));
		else
			namebuf.convert(m_host->GetScriptInfo().name);

		guidstring.convert(pfc::print_guid(m_host->GetGUID()));
	}	

	if (m_debug_manager) 
	{
		IDebugDocumentHelperPtr helper;

		if (SUCCEEDED(hr)) hr = m_debug_manager->CreateDebugDocumentHelper(NULL, &helper);
		if (SUCCEEDED(hr)) hr = helper->Init(m_debug_application, 
			(!path) ? namebuf.get_ptr() : path, 
			(!path) ? guidstring.get_ptr() : path,
			TEXT_DOC_ATTR_READONLY);
		if (SUCCEEDED(hr)) hr = helper->Attach(NULL);
		if (SUCCEEDED(hr)) hr = helper->AddUnicodeText(code);
		if (SUCCEEDED(hr)) hr = helper->DefineScriptBlock(0, len, m_script_engine, FALSE, &source_context);
		if (SUCCEEDED(hr)) m_debug_docs.find_or_add(source_context) = helper;
	}
	else
	{
		source_context = 0;
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

	if (SUCCEEDED(err->GetExceptionInfo(&excep)))
	{
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
            formatter << "Ln: " << (t_uint32)(line + 1) << ", Col: " << (t_uint32)(charpos + 1) << "\n";
            formatter << string_utf8_from_wide(sourceline);
            if (name.length() > 0) 
                formatter << "\n(at): " << name;
        }
        else
        {
            formatter << "Unknown error code: 0x" << pfc::format_hex_lowercase((unsigned)excep.scode);
        }

		SendMessage(m_host->GetHWND(), UWM_SCRIPT_ERROR, 0, (LPARAM)formatter.get_ptr());

		if (excep.bstrSource)      SysFreeString(excep.bstrSource);
		if (excep.bstrDescription) SysFreeString(excep.bstrDescription);
		if (excep.bstrHelpFile)    SysFreeString(excep.bstrHelpFile);
	}
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

void PanelDropTarget::process_dropped_items::on_completion(const pfc::list_base_const_t<metadb_handle_ptr> & p_items)
{
	bit_array_true selection_them;
	bit_array_false selection_none;
	bit_array * select_ptr = &selection_them;
	static_api_ptr_t<playlist_manager> pm;
	t_size playlist;

	if (m_playlist_idx == -1)
		playlist = pm->get_active_playlist();
	else
		playlist = m_playlist_idx;

	if (!m_to_select)
		select_ptr = &selection_none;

	if (playlist != pfc_infinite && playlist < pm->get_playlist_count())
	{
		pm->playlist_add_items(playlist, p_items, *select_ptr);
	}
}

STDMETHODIMP PanelDropTarget::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	if (!pdwEffect) return E_POINTER;

	m_action->Reset();
	// Parsable?
	m_action->Parsable() = static_api_ptr_t<playlist_incoming_item_filter>()->process_dropped_files_check_ex(pDataObj, &m_effect);

	ScreenToClient(m_host->GetHWND(), reinterpret_cast<LPPOINT>(&pt));
	MessageParam param = {grfKeyState, pt.x, pt.y, m_action };
	SendMessage(m_host->GetHWND(), UWM_DRAG_ENTER, 0, (LPARAM)&param);

	if (!m_action->Parsable())
		*pdwEffect = DROPEFFECT_NONE;
	else
		*pdwEffect = m_effect;
	return S_OK;
}

STDMETHODIMP PanelDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	if (!pdwEffect) return E_POINTER;

	ScreenToClient(m_host->GetHWND(), reinterpret_cast<LPPOINT>(&pt));
	MessageParam param = {grfKeyState, pt.x, pt.y, m_action };
	SendMessage(m_host->GetHWND(), UWM_DRAG_OVER, 0, (LPARAM)&param);

	if (!m_action->Parsable())
		*pdwEffect = DROPEFFECT_NONE;
	else
		*pdwEffect = m_effect;

	return S_OK;
}

STDMETHODIMP PanelDropTarget::DragLeave()
{
	SendMessage(m_host->GetHWND(), UWM_DRAG_LEAVE, 0, 0);
	return S_OK;
}

STDMETHODIMP PanelDropTarget::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	if (!pdwEffect) return E_POINTER;

	ScreenToClient(m_host->GetHWND(), reinterpret_cast<LPPOINT>(&pt));
	MessageParam param = {grfKeyState, pt.x, pt.y, m_action };
	SendMessage(m_host->GetHWND(), UWM_DRAG_DROP, 0, (LPARAM)&param);

	int playlist = m_action->Playlist();
	bool to_select = m_action->ToSelect();

	if (m_action->Parsable())
	{
		switch (m_action->Mode())
		{
		case DropSourceAction::kActionModePlaylist:
			static_api_ptr_t<playlist_incoming_item_filter_v2>()->process_dropped_files_async(pDataObj, 
				playlist_incoming_item_filter_v2::op_flag_delay_ui,
				core_api::get_main_window(), new service_impl_t<process_dropped_items>(playlist, to_select));
			break;

		default:
			break;
		}
	}

	if (!m_action->Parsable())
		*pdwEffect = DROPEFFECT_NONE;
	else
		*pdwEffect = m_effect;

	return S_OK;
}

void wsh_panel_window::update_script(const char * name /*= NULL*/, const char * code /*= NULL*/)
{
	if (name && code)
	{
		get_script_engine() = name;
		get_script_code() = code;
	}

	script_unload();
	script_load();
}

bool wsh_panel_window::script_load()
{
	TRACK_FUNCTION();

	helpers::mm_timer timer;
	bool result = true;
	timer.start();

	// Set window edge
	{
		DWORD extstyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);

		// Exclude all edge style
		extstyle &= ~WS_EX_CLIENTEDGE & ~WS_EX_STATICEDGE;
		extstyle |= edge_style_from_config(get_edge_style());
		SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, extstyle);
		SetWindowPos(m_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	// Set something to default
	m_max_size.x = INT_MAX;
	m_max_size.y = INT_MAX;
	m_min_size.x = 0;
	m_min_size.x = 0;
	PostMessage(m_hwnd, UWM_SIZELIMITECHANGED, 0, uie::size_limit_all);

	if (get_disabled_before())
	{
		PostMessage(m_hwnd, UWM_SCRIPT_DISABLED_BEFORE, 0, 0);
		return false;
	}

	HRESULT hr = m_script_host->Initialize();

	if (FAILED(hr))
	{
		// Format error message
		pfc::string_simple win32_error_msg = format_win32_error(hr);
		pfc::string_formatter msg_formatter;

		msg_formatter << "Scripting Engine Initialization Failed ("
			<< GetScriptInfo().build_info_string() << ", CODE: 0x" 
			<< pfc::format_hex_lowercase((unsigned)hr);

		if (hr != E_UNEXPECTED && hr != _HRESULT_TYPEDEF_(0x80020101L) && hr != _HRESULT_TYPEDEF_(0x86664004L))
		{
			msg_formatter << "): " << win32_error_msg;
		}
		else
		{
			msg_formatter << ")\nCheck the console for more information (Always caused by unexcepted script error).";
		}

		// Show error message
		popup_msg::g_show(msg_formatter, WSPM_NAME, popup_message::icon_error);
		result = false;
	}
	else
	{
		if (GetScriptInfo().feature_mask & t_script_info::kFeatureDragDrop)
		{
			// Ole Drag and Drop support
			RegisterDragDrop(m_hwnd, &m_drop_target);
			m_is_droptarget_registered = true;
		}

		// HACK: Script update will not call on_size, so invoke it explicitly
		SendMessage(m_hwnd, UWM_SIZE, 0, 0);

		// Show init message
		console::formatter() << WSPM_NAME " (" 
			<< GetScriptInfo().build_info_string()
			<< "): initialized in "
			<< (int)(timer.query() * 1000)
			<< " ms";
	}

	return result;
}

void wsh_panel_window::script_unload()
{
	m_script_host->Finalize();

	if (m_is_droptarget_registered)
	{
		RevokeDragDrop(m_hwnd);
		m_is_droptarget_registered = false;
	}

	m_watched_handle.release();
	m_selection_holder.release();
}

void wsh_panel_window::create_context()
{
	if (m_gr_bmp || m_gr_bmp_bk)
		delete_context();

	m_gr_bmp = CreateCompatibleBitmap(m_hdc, m_width, m_height);

	if (get_pseudo_transparent())
	{
		m_gr_bmp_bk = CreateCompatibleBitmap(m_hdc, m_width, m_height);
	}
}

void wsh_panel_window::delete_context()
{
	if (m_gr_bmp)
	{
		DeleteBitmap(m_gr_bmp);
		m_gr_bmp = NULL;
	}

	if (m_gr_bmp_bk)
	{
		DeleteBitmap(m_gr_bmp_bk);
		m_gr_bmp_bk = NULL;
	}
}

ui_helpers::container_window::class_data & wsh_panel_window::get_class_data() const
{
	static ui_helpers::container_window::class_data my_class_data =
	{
		_T("uie_wsh_panel_mod_class"), 
		_T(""), 
		0, 
		false, 
		false, 
		0, 
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 
		edge_style_from_config(get_edge_style()),
		CS_DBLCLKS,
		true, true, true, IDC_ARROW
	};

	return my_class_data;
}

bool wsh_panel_window::show_configure_popup(HWND parent)
{
	modal_dialog_scope scope;
	if (!scope.can_create()) return false;
	scope.initialize(parent);

	CDialogConf dlg(this);
	return (dlg.DoModal(parent) == IDOK);
}

bool wsh_panel_window::show_property_popup(HWND parent)
{
	modal_dialog_scope scope;
	if (!scope.can_create()) return false;
	scope.initialize(parent);

	CDialogProperty dlg(this);
	return (dlg.DoModal(parent) == IDOK);
}

LRESULT wsh_panel_window::on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	//TRACK_CALL_TEXT_FORMAT("uie_win::on_message(hwnd=0x%x,msg=%u,wp=%d,lp=%d)", m_hwnd, msg, wp, lp);

	switch (msg)
	{
	case WM_CREATE:
		{
			RECT rect;
			m_hwnd = hwnd;
			m_hdc = GetDC(m_hwnd);
			GetClientRect(m_hwnd, &rect);
			m_width  = rect.right - rect.left;
			m_height = rect.bottom - rect.top;
			create_context();
			// Interfaces
			m_gr_wrap.Attach(new com_object_impl_t<GdiGraphics>(), false);
			panel_notifier_manager::instance().add_window(m_hwnd);
			if (get_delay_load())
				delay_loader::g_enqueue(new delay_script_init_action(m_hwnd));
			else
				script_load();
		}
		return 0;

	case WM_DESTROY:
		script_unload();
		panel_notifier_manager::instance().remove_window(m_hwnd);
		if (m_gr_wrap)
			m_gr_wrap.Release();
		delete_context();
		ReleaseDC(m_hwnd, m_hdc);
		return 0;

	case UWM_SCRIPT_INIT:
		script_load();
		return 0;

	case UWM_SCRIPT_TERM:
		script_unload();
		return 0;

	case WM_DISPLAYCHANGE:
	case WM_THEMECHANGED:
		update_script();
		return 0;

	case WM_ERASEBKGND:
		if (get_pseudo_transparent())
			PostMessage(m_hwnd, UWM_REFRESHBK, 0, 0);
		return 1;

	case UWM_REFRESHBK:
		Redraw();
		return 0;

	case UWM_REFRESHBKDONE:
		on_refresh_background_done();
		return 0;

	case WM_PAINT:
		{
			if (m_suppress_drawing)
				break;

			if (get_pseudo_transparent() && !m_paint_pending)
			{
				RECT rc;

				GetUpdateRect(m_hwnd, &rc, FALSE);
				RefreshBackground(&rc);
				return 0;
			}

			PAINTSTRUCT ps;
			HDC dc = BeginPaint(m_hwnd, &ps);
			on_paint(dc, &ps.rcPaint);
			EndPaint(m_hwnd, &ps);
			m_paint_pending = false;
		}
		return 0;

	case UWM_SIZE:
		on_size(m_width, m_height);
		if (get_pseudo_transparent())
			PostMessage(m_hwnd, UWM_REFRESHBK, 0, 0);
		else
			Repaint();
		return 0;

	case WM_SIZE:
		{
			RECT rect;
			GetClientRect(m_hwnd, &rect);
			on_size(rect.right - rect.left, rect.bottom - rect.top);
			if (get_pseudo_transparent())
				PostMessage(m_hwnd, UWM_REFRESHBK, 0, 0);
			else
				Repaint();
		}
		return 0;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pmmi = reinterpret_cast<LPMINMAXINFO>(lp);
			memcpy(&pmmi->ptMaxTrackSize, &GetMaxSize(), sizeof(POINT));
			memcpy(&pmmi->ptMinTrackSize, &GetMinSize(), sizeof(POINT));
		}
		return 0;

	case WM_GETDLGCODE:
		return GetDlgCode();

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		on_mouse_button_down(msg, wp, lp);
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		if (on_mouse_button_up(msg, wp, lp))
			return 0;
		break;

	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
		on_mouse_button_dblclk(msg, wp, lp);
		break;

	case WM_CONTEXTMENU:
		on_context_menu(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
		return 1;

	case WM_MOUSEMOVE:
		on_mouse_move(wp, lp);
		break;

	case WM_MOUSELEAVE:
		on_mouse_leave();
		break;

	case WM_MOUSEWHEEL:
		on_mouse_wheel(wp);
		break;

	case WM_SETCURSOR:
		return 1;

	case WM_KEYDOWN:
		{
			static_api_ptr_t<keyboard_shortcut_manager_v2>  ksm;

			if (ksm->process_keydown_simple(wp))
				return 0;

			VARIANTARG args[1];

			args[0].vt = VT_UI4;
			args[0].ulVal = (ULONG)wp;

			script_invoke_v(L"on_key_down", args, _countof(args));
		}
		return 0;

	case WM_KEYUP:
		{
			VARIANTARG args[1];

			args[0].vt = VT_UI4;
			args[0].ulVal = (ULONG)wp;

			script_invoke_v(L"on_key_up", args, _countof(args));
		}
		return 0;

	case WM_CHAR:
		{
			VARIANTARG args[1];

			args[0].vt = VT_UI4;
			args[0].ulVal = (ULONG)wp;
			script_invoke_v(L"on_char", args, _countof(args));
		}
		return 0;

	case WM_SETFOCUS:
		{
			PreserveSelection();

			VARIANTARG args[1];

			args[0].vt = VT_BOOL;
			args[0].boolVal = VARIANT_TRUE;
			script_invoke_v(L"on_focus", args, _countof(args));
		}
		break;

	case WM_KILLFOCUS:
		{
			m_selection_holder.release();

			VARIANTARG args[1];

			args[0].vt = VT_BOOL;
			args[0].boolVal = VARIANT_FALSE;
			script_invoke_v(L"on_focus", args, _countof(args));
		}
		break;

	case UWM_SCRIPT_ERROR_TIMEOUT:
		get_disabled_before() = true;
		//script_unload();
		m_script_host->Stop();

		popup_msg::g_show(pfc::string_formatter() << "Script terminated due to the panel (" 
			<< GetScriptInfo().build_info_string()
			<< ") seems to be unresponsive, "
			<< "please check your script (usually infinite loop).", 
			WSPM_NAME, popup_message::icon_error);

		Repaint();
		return 0;

	case UWM_SCRIPT_ERROR:
		//script_unload();
		m_script_host->Stop();

		if (lp)
		{
			console::error(reinterpret_cast<const char *>(lp));
			MessageBeep(MB_ICONASTERISK);
		}

		Repaint();
		return 0;

	case UWM_SCRIPT_DISABLED_BEFORE:
		// Show error message
		popup_msg::g_show(pfc::string_formatter() 
			<< "Panel ("
			<< GetScriptInfo().build_info_string()
			<< "): Refuse to load script due to critical error last run,"
			<< " please check your script and apply it again.",
			WSPM_NAME, 
			popup_message::icon_error);
		return 0;

	case UWM_TIMER:
		on_timer(wp);
		return 0;

	case UWM_SHOWCONFIGURE:
		show_configure_popup(m_hwnd);
		return 0;

	case UWM_SHOWPROPERTIES:
		show_property_popup(m_hwnd);
		return 0;

	case UWM_DRAG_ENTER:
		on_drag_enter(lp);
		return 0;

	case UWM_DRAG_OVER:
		on_drag_over(lp);
		return 0;

	case UWM_DRAG_LEAVE:
		on_drag_leave();
		return 0;

	case UWM_DRAG_DROP:
		on_drag_drop(lp);
		return 0;

	case CALLBACK_UWM_PLAYLIST_STOP_AFTER_CURRENT:
		on_playlist_stop_after_current_changed(wp);
		return 0;

	case CALLBACK_UWM_CURSOR_FOLLOW_PLAYBACK:
		on_cursor_follow_playback_changed(wp);
		return 0;

	case CALLBACK_UWM_PLAYBACK_FOLLOW_CURSOR:
		on_playback_follow_cursor_changed(wp);
		return 0;

	case CALLBACK_UWM_NOTIFY_DATA:
		on_notify_data(wp);
		return 0;

	case CALLBACK_UWM_GETALBUMARTASYNCDONE:
		on_get_album_art_done(lp);
		return 0;

	case CALLBACK_UWM_LOADIMAGEASYNCDONE:
		on_load_image_done(lp);
		return 0;

	case CALLBACK_UWM_FONT_CHANGED:
		on_font_changed();
		return 0;

	case CALLBACK_UWM_COLORS_CHANGED:
		on_colors_changed();
		return 0;

	case CALLBACK_UWM_ON_ITEM_PLAYED:
		on_item_played(wp);
		return 0;

	case CALLBACK_UWM_ON_CHANGED_SORTED:
		on_changed_sorted(wp);
		return 0;

	case CALLBACK_UWM_ON_SELECTION_CHANGED:
		on_selection_changed(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_STARTING:
		on_playback_starting((playback_control::t_track_command)wp, lp != 0);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_NEW_TRACK:
		on_playback_new_track(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_STOP:
		on_playback_stop((playback_control::t_stop_reason)wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_SEEK:
		on_playback_seek(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_PAUSE:
		on_playback_pause(wp != 0);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_EDITED:
		on_playback_edited(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO:
		on_playback_dynamic_info();
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO_TRACK:
		on_playback_dynamic_info_track();
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_TIME:
		on_playback_time(wp);
		return 0;

	case CALLBACK_UWM_ON_VOLUME_CHANGE:
		on_volume_change(wp);
		return 0;

	case CALLBACK_UWM_ON_ITEM_FOCUS_CHANGE:
		on_item_focus_change();
		return 0;

	case CALLBACK_UWM_ON_PLAYBACK_ORDER_CHANGED:
		on_playback_order_changed((t_size)wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYLIST_SWITCH:
		on_playlist_switch();
		return 0;

	case CALLBACK_UWM_ON_PLAYLISTS_CHANGED:
		on_playlists_changed();
		return 0;

	case CALLBACK_UWM_ON_PLAYLIST_ITEMS_ADDED:
		on_playlist_items_added(wp);
		return 0;

	case CALLBACK_UWM_ON_PLAYLIST_ITEMS_REMOVED:
		on_playlist_items_removed(wp, lp);
		return 0;

	case CALLBACK_UWM_ON_PLAYLIST_ITEMS_SELECTION_CHANGE:
		on_playlist_items_selection_change();
        return 0;

    case CALLBACK_UWM_ON_PLAYBACK_QUEUE_CHANGED:
        on_playback_queue_changed(wp);
        return 0;
	}

	return uDefWindowProc(hwnd, msg, wp, lp);
}

void wsh_panel_window::on_size(int w, int h)
{
	TRACK_FUNCTION();

	m_width = w;
	m_height = h;

	delete_context();
	create_context();

	script_invoke_v(L"on_size");
}

void wsh_panel_window::on_paint(HDC dc, LPRECT lpUpdateRect)
{
	TRACK_FUNCTION();

	if (!dc || !lpUpdateRect || !m_gr_bmp || !m_gr_wrap)
		return;

	HDC memdc = CreateCompatibleDC(dc);
	HBITMAP oldbmp = SelectBitmap(memdc, m_gr_bmp);
	
	if (!m_script_host->HasError())
	{
		if (get_pseudo_transparent())
		{
			HDC bkdc = CreateCompatibleDC(dc);
			HBITMAP bkoldbmp = SelectBitmap(bkdc, m_gr_bmp_bk);

			BitBlt(memdc, lpUpdateRect->left, lpUpdateRect->top, 
				lpUpdateRect->right - lpUpdateRect->left, 
				lpUpdateRect->bottom - lpUpdateRect->top, bkdc, lpUpdateRect->left, lpUpdateRect->top, SRCCOPY);

			SelectBitmap(bkdc, bkoldbmp);
			DeleteDC(bkdc);
		}
		else
		{
			RECT rc = {0, 0, m_width, m_height};

			FillRect(memdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
		}

		if (m_script_host->Ready())
		{
			// Prepare graphics object to the script.
			Gdiplus::Graphics gr(memdc);
			Gdiplus::Rect rect(lpUpdateRect->left, lpUpdateRect->top,
				lpUpdateRect->right - lpUpdateRect->left,
				lpUpdateRect->bottom - lpUpdateRect->top);

			// SetClip() may improve performance slightly
			gr.SetClip(rect);

			m_gr_wrap->put__ptr(&gr);

			{
				VARIANTARG args[1];

				args[0].vt = VT_DISPATCH;
				args[0].pdispVal = m_gr_wrap;
				script_invoke_v(L"on_paint", args, _countof(args));
			}

			m_gr_wrap->put__ptr(NULL);
		}
	}
	else
	{
		const TCHAR errmsg[] = _T("Aw, crashed :(");
		RECT rc = {0, 0, m_width, m_height};
		SIZE sz = {0};

		// Font chosing
		HFONT newfont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, _T("Tahoma"));
		HFONT oldfont = (HFONT)SelectObject(memdc, newfont);

		// Font drawing
		{
			LOGBRUSH lbBack = {BS_SOLID, RGB(35, 48, 64), 0};
			HBRUSH hBack = CreateBrushIndirect(&lbBack);

			FillRect(memdc, &rc, hBack);
			SetBkMode(memdc, TRANSPARENT);

			SetTextColor(memdc, RGB(255, 255, 255));
			DrawText(memdc, errmsg, -1, &rc, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE);

			DeleteObject(hBack);
		}

		SelectObject(memdc, oldfont);
	}

	BitBlt(dc, 0, 0, m_width, m_height, memdc, 0, 0, SRCCOPY);
	SelectBitmap(memdc, oldbmp);
	DeleteDC(memdc);
}

void wsh_panel_window::on_timer(UINT timer_id)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_UI4;
	args[0].ulVal = timer_id;
	script_invoke_v(L"on_timer", args, _countof(args));
}

void wsh_panel_window::on_context_menu(int x, int y)
{
	const int base_id = 0;
	HMENU menu = CreatePopupMenu();
	int ret = 0;

	build_context_menu(menu, x, y, base_id);
	ret = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, x, y, 0, m_hwnd, 0);
	execute_context_menu_command(ret, base_id);
	DestroyMenu(menu);
}

void wsh_panel_window::on_mouse_wheel(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_I4;
	args[0].lVal = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
	script_invoke_v(L"on_mouse_wheel", args, _countof(args));
}

void wsh_panel_window::on_mouse_leave()
{
	TRACK_FUNCTION();

	m_is_mouse_tracked = false;

	script_invoke_v(L"on_mouse_leave");
	// Restore default cursor
	SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
}

void wsh_panel_window::on_mouse_move(WPARAM wp, LPARAM lp)
{
	TRACK_FUNCTION();

	if (!m_is_mouse_tracked)
	{
		TRACKMOUSEEVENT tme;

		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hwnd;
		tme.dwFlags = TME_LEAVE;
		TrackMouseEvent(&tme);
		m_is_mouse_tracked = true;

		// Restore default cursor
		SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
	}

	VARIANTARG args[3];

	args[0].vt = VT_I4;
	args[0].lVal = wp;
	args[1].vt = VT_I4;
	args[1].lVal = GET_Y_LPARAM(lp);
	args[2].vt = VT_I4;
	args[2].lVal = GET_X_LPARAM(lp);	
	script_invoke_v(L"on_mouse_move", args, _countof(args));
}

void wsh_panel_window::on_mouse_button_dblclk(UINT msg, WPARAM wp, LPARAM lp)
{
	TRACK_FUNCTION();

	VARIANTARG args[3];

	args[0].vt = VT_I4;
	args[0].lVal = wp;
	args[1].vt = VT_I4;
	args[1].lVal = GET_Y_LPARAM(lp);
	args[2].vt = VT_I4;
	args[2].lVal = GET_X_LPARAM(lp);

	switch (msg)
	{
	case WM_LBUTTONDBLCLK:
		script_invoke_v(L"on_mouse_lbtn_dblclk", args, _countof(args));
		break;

	case WM_MBUTTONDBLCLK:
		script_invoke_v(L"on_mouse_mbtn_dblclk", args, _countof(args));
		break;

	case WM_RBUTTONDBLCLK:
		script_invoke_v(L"on_mouse_rbtn_dblclk", args, _countof(args));
		break;
	}
}

bool wsh_panel_window::on_mouse_button_up(UINT msg, WPARAM wp, LPARAM lp)
{
	TRACK_FUNCTION();

	bool ret = false;
	VARIANTARG args[3];
	args[0].vt = VT_I4;
	args[0].lVal = wp;
	args[1].vt = VT_I4;
	args[1].lVal = GET_Y_LPARAM(lp);
	args[2].vt = VT_I4;
	args[2].lVal = GET_X_LPARAM(lp);	

	switch (msg)
	{
	case WM_LBUTTONUP:
		script_invoke_v(L"on_mouse_lbtn_up", args, _countof(args));
		break;

	case WM_MBUTTONUP:
		script_invoke_v(L"on_mouse_mbtn_up", args, _countof(args));
		break;

	case WM_RBUTTONUP:
		{
			_variant_t result;

			if (IsKeyPressed(VK_LSHIFT) && IsKeyPressed(VK_LWIN))
			{
				// HACK: Start debugger manually
				if (IsKeyPressed(VK_LCONTROL) && IsKeyPressed(VK_LMENU))
				{
					m_script_host->StartDebugger();
				}

				break;
			}

			if (SUCCEEDED(script_invoke_v(L"on_mouse_rbtn_up", args, _countof(args), &result)))
			{
				result.ChangeType(VT_BOOL);
				if ((result.boolVal != VARIANT_FALSE))
					ret = true;
			}
		}
		break;
	}

	ReleaseCapture();
	return ret;
}

void wsh_panel_window::on_mouse_button_down(UINT msg, WPARAM wp, LPARAM lp)
{
	TRACK_FUNCTION();

	if (get_grab_focus())
		SetFocus(m_hwnd);

	SetCapture(m_hwnd);

	VARIANTARG args[3];

	args[0].vt = VT_I4;
	args[0].lVal = wp;
	args[1].vt = VT_I4;
	args[1].lVal = GET_Y_LPARAM(lp);
	args[2].vt = VT_I4;
	args[2].lVal = GET_X_LPARAM(lp);

	switch (msg)
	{
	case WM_LBUTTONDOWN:
		script_invoke_v(L"on_mouse_lbtn_down", args, _countof(args));
		break;

	case WM_MBUTTONDOWN:
		script_invoke_v(L"on_mouse_mbtn_down", args, _countof(args));
		break;

	case WM_RBUTTONDOWN:
		script_invoke_v(L"on_mouse_rbtn_down", args, _countof(args));
		break;
	}
}

void wsh_panel_window::on_refresh_background_done()
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_refresh_background_done");
}

void wsh_panel_window::build_context_menu(HMENU menu, int x, int y, int id_base)
{
	::AppendMenu(menu, MF_STRING, id_base + 1, _T("&Properties"));
	::AppendMenu(menu, MF_STRING, id_base + 2, _T("&Configure..."));
}

void wsh_panel_window::execute_context_menu_command(int id, int id_base)
{
	switch (id - id_base)
	{
	case 1:
		show_property_popup(m_hwnd);
		break;

	case 2:
		show_configure_popup(m_hwnd);			
		break;
	}
}

void wsh_panel_window::on_item_played(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<metadb_handle_ptr> > data(wp);

	FbMetadbHandle * handle = new com_object_impl_t<FbMetadbHandle>(data->m_item);
	VARIANTARG args[1];

	args[0].vt = VT_DISPATCH;
	args[0].pdispVal = handle;
	script_invoke_v(L"on_item_played", args, _countof(args));

	if (handle)
		handle->Release();
}

void wsh_panel_window::on_get_album_art_done(LPARAM lp)
{
	TRACK_FUNCTION();

	using namespace helpers;
	album_art_async::t_param * param = reinterpret_cast<album_art_async::t_param *>(lp);
	VARIANTARG args[4];

	args[0].vt = VT_BSTR;
	args[0].bstrVal = SysAllocString(param->image_path);
	args[1].vt = VT_DISPATCH;
	args[1].pdispVal = param->bitmap;
	args[2].vt = VT_I4;
	args[2].lVal = param->art_id;
	args[3].vt = VT_DISPATCH;
	args[3].pdispVal = param->handle;
	script_invoke_v(L"on_get_album_art_done", args, _countof(args));
}

void wsh_panel_window::on_load_image_done(LPARAM lp)
{
	TRACK_FUNCTION();

	using namespace helpers;
	load_image_async::t_param * param = reinterpret_cast<load_image_async::t_param *>(lp);
	VARIANTARG args[3];

	args[0].vt = VT_BSTR;
	args[0].bstrVal = param->path;
	args[1].vt = VT_DISPATCH;
	args[1].pdispVal = param->bitmap;
	args[2].vt = VT_I4;
	args[2].lVal = param->cookie;
	script_invoke_v(L"on_load_image_done", args, _countof(args));
}

void wsh_panel_window::on_playlist_stop_after_current_changed(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(wp);
	script_invoke_v(L"on_playlist_stop_after_current_changed", args, _countof(args));
}

void wsh_panel_window::on_cursor_follow_playback_changed(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(wp);
	script_invoke_v(L"on_cursor_follow_playback_changed", args, _countof(args));
}

void wsh_panel_window::on_playback_follow_cursor_changed(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(wp);
	script_invoke_v(L"on_playback_follow_cursor_changed", args, _countof(args));
}

void wsh_panel_window::on_notify_data(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[2];
	simple_callback_data_scope_releaser<simple_callback_data_2<_bstr_t, _variant_t> > data(wp);

	args[0] = data->m_item2;
	args[1].vt = VT_BSTR;
	args[1].bstrVal = data->m_item1;
	script_invoke_v(L"on_notify_data", args, _countof(args));
}

void wsh_panel_window::on_font_changed()
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_font_changed");
}

void wsh_panel_window::on_colors_changed()
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_colors_changed");
}

void wsh_panel_window::on_playback_starting(play_control::t_track_command cmd, bool paused)
{
	TRACK_FUNCTION();

	VARIANTARG args[2];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(paused);
	args[1].vt = VT_I4;
	args[1].lVal = cmd;
	script_invoke_v(L"on_playback_starting", args, _countof(args));
}

void wsh_panel_window::on_playback_new_track(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<metadb_handle_ptr> > data(wp);
	VARIANTARG args[1];
	FbMetadbHandle * handle = new com_object_impl_t<FbMetadbHandle>(data->m_item);
	
	args[0].vt = VT_DISPATCH;
	args[0].pdispVal = handle;
	script_invoke_v(L"on_playback_new_track", args, _countof(args));

	if (handle)
		handle->Release();
}

void wsh_panel_window::on_playback_stop(play_control::t_stop_reason reason)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_I4;
	args[0].lVal = reason;
	script_invoke_v(L"on_playback_stop", args, _countof(args));
}

void wsh_panel_window::on_playback_seek(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<double> > data(wp);

	VARIANTARG args[1];

	args[0].vt = VT_R8;
	args[0].dblVal = data->m_item;
	script_invoke_v(L"on_playback_seek", args, _countof(args));
}

void wsh_panel_window::on_playback_pause(bool state)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(state);
	script_invoke_v(L"on_playback_pause", args, _countof(args));
}

void wsh_panel_window::on_playback_edited(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<metadb_handle_ptr> > data(wp);
	FbMetadbHandle * handle = new com_object_impl_t<FbMetadbHandle>(data->m_item);
	VARIANTARG args[1];
	
	args[0].vt = VT_DISPATCH;
	args[0].pdispVal = handle;

	script_invoke_v(L"on_playback_edited");
	
	if (handle)
	{
		handle->Release();
	}
}

void wsh_panel_window::on_playback_dynamic_info()
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_playback_dynamic_info");
}

void wsh_panel_window::on_playback_dynamic_info_track()
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_playback_dynamic_info_track");
}

void wsh_panel_window::on_playback_time(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<double> > data(wp);

	VARIANTARG args[1];

	args[0].vt = VT_R8;
	args[0].dblVal = data->m_item;
	script_invoke_v(L"on_playback_time", args, _countof(args));
}

void wsh_panel_window::on_volume_change(WPARAM wp)
{
	TRACK_FUNCTION();

	simple_callback_data_scope_releaser<simple_callback_data<float> > data(wp);

	VARIANTARG args[1];

	args[0].vt = VT_R4;
	args[0].fltVal = data->m_item;
	script_invoke_v(L"on_volume_change", args, _countof(args));
}

void wsh_panel_window::on_item_focus_change()
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_item_focus_change");
}

void wsh_panel_window::on_playback_order_changed(t_size p_new_index)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_I4;
	args[0].lVal = p_new_index;
	script_invoke_v(L"on_playback_order_changed", args, _countof(args));
}

void wsh_panel_window::on_playlist_switch()
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_playlist_switch");
}

void wsh_panel_window::on_playlists_changed()
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_playlists_changed");
}

void wsh_panel_window::on_playlist_items_added(WPARAM wp)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];
	args[0].vt = VT_UI4;
	args[0].ulVal = wp;
	script_invoke_v(L"on_playlist_items_added", args, _countof(args));
}

void wsh_panel_window::on_playlist_items_removed(WPARAM wp, LPARAM lp)
{
	TRACK_FUNCTION();

	VARIANTARG args[2];
	args[0].vt = VT_UI4;
	args[0].ulVal = lp;
	args[1].vt = VT_UI4;
	args[1].ulVal = wp;
	script_invoke_v(L"on_playlist_items_removed", args, _countof(args));
}

void wsh_panel_window::on_playlist_items_selection_change()
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_playlist_items_selection_change");
}

void wsh_panel_window::on_changed_sorted(WPARAM wp)
{
	TRACK_FUNCTION();

	if (GetScriptInfo().feature_mask & t_script_info::kFeatureNoWatchMetadb)
		return;

	simple_callback_data_scope_releaser<nonautoregister_callbacks::t_on_changed_sorted_data> data(wp);

	if (m_watched_handle.is_empty())
		return;

	VARIANTARG args[2];
	IDispatch * handle = NULL;

	if (GetScriptInfo().feature_mask & t_script_info::kFeatureMetadbHandleList0)
	{
		handle = new com_object_impl_t<FbMetadbHandleList>(data->m_items_sorted);
	}
	else
	{
		if (!data->m_items_sorted.have_item(m_watched_handle))
			return;

		handle = new com_object_impl_t<FbMetadbHandle>(m_watched_handle);
	}

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(data->m_fromhook);
	args[1].vt = VT_DISPATCH;
	args[1].pdispVal = handle;
	script_invoke_v(L"on_metadb_changed", args, _countof(args));
	
	if (handle)
		handle->Release();
}

void wsh_panel_window::on_selection_changed(WPARAM wp)
{
	TRACK_FUNCTION();
	
	if (wp != 0)
	{
		if (GetScriptInfo().feature_mask & t_script_info::kFeatureMetadbHandleList0)
		{
			script_invoke_v(L"on_selection_changed");
		}
		else
		{
			IDispatch * handle = NULL;
			simple_callback_data_scope_releaser<simple_callback_data<metadb_handle_ptr> > data(wp);
			handle = new com_object_impl_t<FbMetadbHandle>(data->m_item);

			VARIANTARG args[1];

			args[0].vt = VT_DISPATCH;
			args[0].pdispVal = handle;
			script_invoke_v(L"on_selection_changed", args, _countof(args));

			if (handle)
				handle->Release();
		}
	}
}

void wsh_panel_window::on_drag_enter(LPARAM lp)
{
	TRACK_FUNCTION();

	PanelDropTarget::MessageParam * param = reinterpret_cast<PanelDropTarget::MessageParam *>(lp);
	VARIANTARG args[4];
	args[0].vt = VT_I4;
	args[0].lVal = param->key_state;
	args[1].vt = VT_I4;
	args[1].lVal = param->y;
	args[2].vt = VT_I4;
	args[2].lVal = param->x;	
	args[3].vt = VT_DISPATCH;
	args[3].pdispVal = param->action;
	script_invoke_v(L"on_drag_enter", args, _countof(args));
}

void wsh_panel_window::on_drag_over(LPARAM lp)
{
	TRACK_FUNCTION();

	PanelDropTarget::MessageParam * param = reinterpret_cast<PanelDropTarget::MessageParam *>(lp);
	VARIANTARG args[4];
	args[0].vt = VT_I4;
	args[0].lVal = param->key_state;
	args[1].vt = VT_I4;
	args[1].lVal = param->y;
	args[2].vt = VT_I4;
	args[2].lVal = param->x;	
	args[3].vt = VT_DISPATCH;
	args[3].pdispVal = param->action;
	script_invoke_v(L"on_drag_over", args, _countof(args));
}

void wsh_panel_window::on_drag_leave()
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_drag_leave");
}

void wsh_panel_window::on_drag_drop(LPARAM lp)
{
	TRACK_FUNCTION();

	PanelDropTarget::MessageParam * param = reinterpret_cast<PanelDropTarget::MessageParam *>(lp);
	VARIANTARG args[4];
	args[0].vt = VT_I4;
	args[0].lVal = param->key_state;
	args[1].vt = VT_I4;
	args[1].lVal = param->y;
	args[2].vt = VT_I4;
	args[2].lVal = param->x;	
	args[3].vt = VT_DISPATCH;
	args[3].pdispVal = param->action;
	script_invoke_v(L"on_drag_drop", args, _countof(args));
}

void wsh_panel_window::on_playback_queue_changed(WPARAM wp)
{
    TRACK_FUNCTION();

    VARIANTARG args[1];
    args[0].vt = VT_I4;
    args[0].lVal = wp;
    script_invoke_v(L"on_playback_queue_changed", args, _countof(args));
}

const GUID& wsh_panel_window_cui::get_extension_guid() const
{
	return g_wsh_panel_window_extension_guid;
}

void wsh_panel_window_cui::get_name(pfc::string_base& out) const
{
	out = WSPM_NAME;
}

void wsh_panel_window_cui::get_category(pfc::string_base& out) const
{
	out = "Panels";
}

unsigned wsh_panel_window_cui::get_type() const
{
	return uie::type_toolbar | uie::type_panel;
}

void wsh_panel_window_cui::set_config(stream_reader * reader, t_size size, abort_callback & abort)
{
	load_config(reader, size, abort);
}

void wsh_panel_window_cui::get_config(stream_writer * writer, abort_callback & abort) const
{
	save_config(writer, abort);
}

bool wsh_panel_window_cui::have_config_popup() const
{
	return true;
}

bool wsh_panel_window_cui::show_config_popup(HWND parent)
{
	return show_configure_popup(parent);
}

LRESULT wsh_panel_window_cui::on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CREATE:
		try
		{
			static_api_ptr_t<columns_ui::fonts::manager>()->register_common_callback(this);
			static_api_ptr_t<columns_ui::colours::manager>()->register_common_callback(this);
		}
		catch (exception_service_not_found &)
		{
			// Using in Default UI and dockable panels without Columns UI installed?
			static bool g_reported = false;
			const char warning[] = "Warning: At least one " WSPM_NAME " instance is running in CUI containers "
								   "(dockable panels, Func UI, etc) without some services provided by the "
								   "Columns UI component (have not been installed or have a very old "
								   "version installed?).\n"
								   "Please download and install the latest version of Columns UI:\n"
								   "http://yuo.be/columns.php";

			if (!g_cfg_cui_warning_reported)
			{
				popup_msg::g_show(pfc::string_formatter(warning) << "\n\n[This popup message will be shown only once]", 
								  WSPM_NAME);

				g_cfg_cui_warning_reported = true;
			}
			else if (!g_reported)
			{
				console::formatter() << "\n" WSPM_NAME ": " << warning << "\n\n";
				g_reported = true;
			}
		}
		catch (...)
		{
		}
		break;

	case WM_DESTROY:
		try
		{
			static_api_ptr_t<columns_ui::fonts::manager>()->deregister_common_callback(this);
			static_api_ptr_t<columns_ui::colours::manager>()->deregister_common_callback(this);
		}
		catch (...)
		{
		}
		break;

	case UWM_SIZELIMITECHANGED:
		notify_size_limit_changed_(lp);
		return 0;
	}

	return t_parent::on_message(hwnd, msg, wp, lp);
}

HWND wsh_panel_window_cui::create_or_transfer_window(HWND parent, const uie::window_host_ptr & host, const ui_helpers::window_position_t & p_position)
{
	if (m_host.is_valid())
	{
		ShowWindow(m_hwnd, SW_HIDE);
		SetParent(m_hwnd, parent);
		m_host->relinquish_ownership(m_hwnd);
		m_host = host;

		SetWindowPos(m_hwnd, NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER);
	}
	else
	{
		m_host = host; //store interface to host
		create(parent, this, p_position);
	}

	return get_wnd();
}

void wsh_panel_window_cui::notify_size_limit_changed_(LPARAM lp)
{
	get_host()->on_size_limit_change(m_hwnd, lp);
}

void wsh_panel_window_cui::on_font_changed(t_size mask) const
{
	PostMessage(m_hwnd, CALLBACK_UWM_FONT_CHANGED, 0, 0);
}

void wsh_panel_window_cui::on_colour_changed(t_size mask) const
{
	PostMessage(m_hwnd, CALLBACK_UWM_COLORS_CHANGED, 0, 0);
}

void wsh_panel_window_cui::on_bool_changed(t_size mask) const
{
	// TODO: may be implemented one day
}

DWORD wsh_panel_window_cui::GetColorCUI(unsigned type, const GUID & guid)
{
	if (type <= columns_ui::colours::colour_active_item_frame)
	{
		columns_ui::colours::helper helper(guid);

		return helpers::convert_colorref_to_argb(
			helper.get_colour((columns_ui::colours::colour_identifier_t)type));
	}

	return 0;
}

HFONT wsh_panel_window_cui::GetFontCUI(unsigned type, const GUID & guid)
{
	if (guid == pfc::guid_null)
	{
		if (type <= columns_ui::fonts::font_type_labels)
		{
			try
			{
				return static_api_ptr_t<columns_ui::fonts::manager>()->get_font((columns_ui::fonts::font_type_t)type);
			}
			catch (exception_service_not_found &)
			{
				return uCreateIconFont();
			}
		}
	}
	else
	{
		columns_ui::fonts::helper helper(guid);
		return helper.get_font();
	}

	return NULL;
}

void wsh_panel_window_dui::initialize_window(HWND parent)
{
	t_parent::create(parent);
}

HWND wsh_panel_window_dui::get_wnd()
{
	return t_parent::get_wnd();
}

void wsh_panel_window_dui::set_configuration(ui_element_config::ptr data)
{
	ui_element_config_parser parser(data);
	abort_callback_dummy abort;

	load_config(&parser.m_stream, parser.get_remaining(), abort);

	// FIX: If window already created, DUI won't destroy it and create it again.
	if (m_hwnd)
	{
		update_script();
	}
}

ui_element_config::ptr wsh_panel_window_dui::g_get_default_configuration()
{
	ui_element_config_builder builder;
	abort_callback_dummy abort;
	wsh_panel_vars vars;

	vars.reset_config();
	vars.save_config(&builder.m_stream, abort);
	return builder.finish(g_get_guid());
}

ui_element_config::ptr wsh_panel_window_dui::get_configuration()
{
	ui_element_config_builder builder;
	abort_callback_dummy abort;

	save_config(&builder.m_stream, abort);
	return builder.finish(g_get_guid());
}

void wsh_panel_window_dui::g_get_name(pfc::string_base & out)
{
	out = WSPM_NAME;
}

pfc::string8 wsh_panel_window_dui::g_get_description()
{
	return "Customizable panel with VBScript and JScript scripting support.";
}

GUID wsh_panel_window_dui::g_get_guid()
{
	return g_wsh_panel_window_dui_guid;
}

GUID wsh_panel_window_dui::get_guid()
{
	return g_get_guid();
}

GUID wsh_panel_window_dui::g_get_subclass()
{
	return ui_element_subclass_utility;
}

GUID wsh_panel_window_dui::get_subclass()
{
	return g_get_subclass();
}

void wsh_panel_window_dui::notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size)
{
	if (p_what == ui_element_notify_edit_mode_changed)
	{
		notify_is_edit_mode_changed_(m_callback->is_edit_mode_enabled());
	}
	else if (p_what == ui_element_notify_font_changed)
	{
		PostMessage(m_hwnd, CALLBACK_UWM_FONT_CHANGED, 0, 0);
	}
	else if (p_what == ui_element_notify_colors_changed)
	{
		PostMessage(m_hwnd, CALLBACK_UWM_COLORS_CHANGED, 0, 0);
	}
}

LRESULT wsh_panel_window_dui::on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{	
	case WM_RBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_CONTEXTMENU:
		if (m_is_edit_mode) 
			return DefWindowProc(hwnd, msg, wp, lp);
		break;

	case UWM_SIZELIMITECHANGED:
		notify_size_limit_changed_(lp);
		return 0;
	}

	return t_parent::on_message(hwnd, msg, wp, lp);
}

void wsh_panel_window_dui::notify_size_limit_changed_(LPARAM lp)
{
	m_callback->on_min_max_info_change();
}

DWORD wsh_panel_window_dui::GetColorDUI(unsigned type)
{
	const GUID * guids[] = {
		&ui_color_text,
		&ui_color_background,
		&ui_color_highlight,
		&ui_color_selection,
	};

	if (type < _countof(guids))
	{
		return helpers::convert_colorref_to_argb(m_callback->query_std_color(*guids[type]));
	}

	return 0;
}

HFONT wsh_panel_window_dui::GetFontDUI(unsigned type)
{
	const GUID * guids[] = {
		&ui_font_default,
		&ui_font_tabs,
		&ui_font_lists,
		&ui_font_playlists,
		&ui_font_statusbar,
		&ui_font_console,
	};

	if (type < _countof(guids))
	{
		return m_callback->query_font_ex(*guids[type]);
	}

	return NULL;
}
