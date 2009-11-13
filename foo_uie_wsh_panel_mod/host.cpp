#include "stdafx.h"
#include "host.h"
#include "resource.h"
#include "helpers.h"
#include "panel_notifier.h"
#include "global_cfg.h"
#include "ui_conf.h"
#include "ui_property.h"
#include "user_message.h"


// Panel
static ui_extension::window_factory<uie_win> g_uie_win;

//#define ALMOST_SUCCEEDED(hr) (SUCCEEDED(hr) || hr == OLESCRIPT_E_SYNTAX)


HostComm::HostComm() 
: m_hwnd(NULL)
, m_hdc(NULL)
, m_width(0)
, m_height(0)
, m_gr_bmp(NULL)
, m_bk_updating(false)
, m_paint_pending(false)
, m_accuracy(0)
{
	TIMECAPS tc;

	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR)
	{
		m_accuracy = min(max(tc.wPeriodMin, 0), tc.wPeriodMax);
	}

	timeBeginPeriod(m_accuracy);
}

HostComm::~HostComm()
{
	timeEndPeriod(m_accuracy);
}

GUID HostComm::GetGUID()
{
	GUID guid;

	SendMessage(m_hwnd, UWM_GETCONFIGGUID, 0, (LPARAM)&guid);
	return guid;
}

HWND HostComm::GetHWND()
{
	return m_hwnd;
}

UINT HostComm::GetWidth()
{
	return m_width;
}

UINT HostComm::GetHeight()
{
	return m_height;
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

void HostComm::OnScriptError(LPCWSTR str)
{
	SendMessage(m_hwnd, UWM_SCRIPT_ERROR, 0, (LPARAM)str);
}

void HostComm::RefreshBackground(LPRECT lprcUpdate /*= NULL*/)
{
	HWND wnd_parent = GetAncestor(m_hwnd, GA_PARENT);

	if (!wnd_parent || IsIconic(core_api::get_main_window()) || !IsWindowVisible(m_hwnd))
		return;

	HDC dc_parent = GetDC(wnd_parent);
	HDC hdc_bk = CreateCompatibleDC(dc_parent);
	POINT pt = { 0, 0};
	RECT rect_child = { 0, 0, m_width, m_height };
	RECT rect_parent;
	HRGN rgn_child = NULL;

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

	MapWindowPoints(m_hwnd, wnd_parent, &pt, 1);
	CopyRect(&rect_parent, &rect_child);
	MapWindowRect(m_hwnd, wnd_parent, &rect_parent);

	// Force Repaint
	m_bk_updating = true;
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
	m_bk_updating = false;
	SendMessage(m_hwnd, UWM_REFRESHBKDONE, 0, 0);
	Repaint(true);
}

ITimerObj * HostComm::CreateTimerTimeout(UINT timeout)
{
	UINT id = timeSetEvent(timeout, m_accuracy, g_timer_proc, (DWORD_PTR)this, TIME_ONESHOT);

	if (id == NULL) return NULL;

	ITimerObj * timer = new com_object_impl_t<TimerObj>(id);
	return timer;
}

ITimerObj * HostComm::CreateTimerInterval(UINT delay)
{
	UINT id = timeSetEvent(delay, m_accuracy, g_timer_proc, (DWORD_PTR)this, TIME_PERIODIC);

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

metadb_handle_ptr & HostComm::GetWatchedMetadbHandle()
{
	return m_watched_handle;
}

IGdiBitmap * HostComm::GetBackgroundImage()
{
	Gdiplus::Bitmap * bitmap = NULL;
	IGdiBitmap * ret = NULL;

	if (get_pseudo_transparent())
	{
		bitmap = Gdiplus::Bitmap::FromHBITMAP(m_gr_bmp_bk, NULL);

		if (!helpers::check_gdiplus_object(bitmap))
		{
			delete bitmap;
			bitmap = NULL;
		}
	}

	if (bitmap)
		ret = new com_object_impl_t<GdiBitmap>(bitmap);

	return ret;
}

void CALLBACK HostComm::g_timer_proc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	HostComm * p = (HostComm *)dwUser;

	PostMessage(p->m_hwnd, UWM_TIMER, uTimerID, 0);
}

STDMETHODIMP FbWindow::get_ID(UINT* p)
{
	TRACK_FUNCTION();

	if (!p ) return E_POINTER;

	*p = (UINT)m_host->GetHWND();
	return S_OK;
}

STDMETHODIMP FbWindow::get_Width(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->GetWidth();
	return S_OK;
}

STDMETHODIMP FbWindow::get_Height(UINT* p)
{
	TRACK_FUNCTION();

	if (!p) return E_POINTER;

	*p = m_host->GetHeight();
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

STDMETHODIMP FbWindow::NotifyOthers(BSTR name, BSTR info)
{
	TRACK_FUNCTION();

	if (!name || !info) return E_INVALIDARG;

	struct notify_callback : public panel_notifier_callback
	{
		BSTR m_name;
		BSTR m_info;

		notify_callback(BSTR p_name, BSTR p_info)
		{
			m_name = SysAllocString(p_name);
			m_info = SysAllocString(p_info);
		}

		virtual ~notify_callback()
		{
			SysFreeString(m_name);
			SysFreeString(m_info);
		}

		virtual void on_callback(HWND hwnd, UINT uMsg, LRESULT lResult) {}
	};

	notify_callback * callback_ptr = new notify_callback(name, info);

	g_get_panel_notifier().notify_others_callback(m_host->GetHWND(), UWM_NOTIFY_DATA, 
		(WPARAM)callback_ptr->m_name, 
		(LPARAM)callback_ptr->m_info, 
		callback_ptr);

	return S_OK;
}

STDMETHODIMP FbWindow::WatchMetadb(IFbMetadbHandle * handle)
{
	TRACK_FUNCTION();

	if (!handle) return E_INVALIDARG;

	metadb_handle * ptr;

	handle->get__ptr((void**)&ptr);

	if (!ptr) return E_INVALIDARG;

	m_host->GetWatchedMetadbHandle() = ptr;
	return S_OK;
}

STDMETHODIMP FbWindow::UnWatchMetadb()
{
	TRACK_FUNCTION();

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

STDMETHODIMP ScriptSite::GetLCID(LCID* plcid)
{
	return E_NOTIMPL;
}

STDMETHODIMP ScriptSite::GetItemInfo(LPCOLESTR name, DWORD mask, IUnknown** ppunk, ITypeInfo** ppti)
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
	}

	return TYPE_E_ELEMENTNOTFOUND;
}

STDMETHODIMP ScriptSite::GetDocVersionString(BSTR* pstr)
{
	return E_NOTIMPL;
}

STDMETHODIMP ScriptSite::OnScriptTerminate(const VARIANT* result, const EXCEPINFO* excep)
{
	return E_NOTIMPL;
}

STDMETHODIMP ScriptSite::OnStateChange(SCRIPTSTATE state)
{
	m_sstate = state;
	return S_OK;
}

STDMETHODIMP ScriptSite::OnScriptError(IActiveScriptError* err)
{
	TRACK_FUNCTION();

	DWORD ctx = 0;
	ULONG line = 0;
	LONG  charpos = 0;
	EXCEPINFO excep = { 0 };
	WCHAR buf[512] = { 0 };
	_bstr_t sourceline;

	if (FAILED(err->GetSourcePosition(&ctx, &line, &charpos)))
	{
		line = 0;
		charpos = 0;
	}

	if (FAILED(err->GetSourceLineText(sourceline.GetAddress())) || !sourceline.length())
	{
		sourceline = L"<no source text available>";
	}

	if (SUCCEEDED(err->GetExceptionInfo(&excep)))
	{
		// Do a deferred fill-in if necessary
		if (excep.pfnDeferredFillIn)
			(*excep.pfnDeferredFillIn)(&excep);

		pfc::stringcvt::string_os_from_utf8 guid_str(pfc::print_guid(m_host->GetGUID()));

		StringCbPrintf(buf, sizeof(buf), _T("WSH Panel Mod (GUID: %s): %s:\n%s\nLn: %d, Col: %d\n%s\n"),
			guid_str.get_ptr(), excep.bstrSource, excep.bstrDescription, line + 1, charpos + 1, sourceline.GetBSTR());

		m_host->OnScriptError(buf);

		SysFreeString(excep.bstrSource);
		SysFreeString(excep.bstrDescription);
		SysFreeString(excep.bstrHelpFile);
	}

	return S_OK;
}

STDMETHODIMP ScriptSite::OnEnterScript()
{
	m_dwStartTime = GetTickCount();
	return S_OK;
}

STDMETHODIMP ScriptSite::OnLeaveScript()
{
	return S_OK;
}

STDMETHODIMP ScriptSite::GetWindow(HWND *phwnd)
{
	*phwnd = m_host->GetHWND();

	return S_OK;
}

STDMETHODIMP ScriptSite::EnableModeless(BOOL fEnable)
{
	return S_OK;
}

STDMETHODIMP ScriptSite::QueryContinue()
{
	if (m_bQueryContinue)
	{
		unsigned timeout = g_cfg_timeout * 1000;
		unsigned delta = GetTickCount() - m_dwStartTime;

		if (timeout != 0 && (delta > timeout))
		{
			SendMessage(m_host->GetHWND(), UWM_SCRIPT_ERROR_TIMEOUT, 0, 0);
			return S_FALSE;
		}
	}

	return S_OK;
}

uie_win::uie_win() 
: m_ismousetracked(false)
, m_script_site(this)
{

}

void uie_win::on_update_script(const char* name, const char* code)
{
	get_script_name() = name;
	get_script_code() = code;
	script_term();
	script_init();
}

HRESULT uie_win::_script_init()
{
	_COM_SMARTPTR_TYPEDEF(IActiveScriptParse, IID_IActiveScriptParse);

	script_term();

	HRESULT hr = S_OK;
	IActiveScriptParsePtr parser;
	pfc::stringcvt::string_wide_from_utf8_fast wname(get_script_name());
	pfc::stringcvt::string_wide_from_utf8_fast wcode(get_script_code());

	if (SUCCEEDED(hr)) hr = m_script_engine.CreateInstance(wname.get_ptr(), NULL, CLSCTX_INPROC_SERVER);
	if (SUCCEEDED(hr)) hr = m_script_engine->SetScriptSite(&m_script_site);
	if (SUCCEEDED(hr)) hr = m_script_engine->QueryInterface(&parser);
	if (SUCCEEDED(hr)) hr = parser->InitNew();

	if (g_cfg_safe_mode)
	{
		_COM_SMARTPTR_TYPEDEF(IObjectSafety, IID_IObjectSafety);

		IObjectSafetyPtr psafe;

		if (SUCCEEDED(m_script_engine->QueryInterface(&psafe)))
			psafe->SetInterfaceSafetyOptions(IID_IDispatch, INTERFACE_USES_SECURITY_MANAGER, INTERFACE_USES_SECURITY_MANAGER);
	}

	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"window", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"gdi", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"fb", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->AddNamedItem(L"utils", SCRIPTITEM_ISVISIBLE);
	if (SUCCEEDED(hr)) hr = m_script_engine->SetScriptState(SCRIPTSTATE_CONNECTED);
	if (SUCCEEDED(hr)) hr = m_script_engine->GetScriptDispatch(NULL, &m_script_root);
	if (SUCCEEDED(hr)) hr = parser->ParseScriptText(wcode.get_ptr(), NULL, NULL, NULL, NULL, 0, 0, NULL, NULL);

	return hr;
}

bool uie_win::script_init()
{
	TRACK_FUNCTION();

	pfc::hires_timer timer;
	timer.start();

	// Set window edge
	{
		DWORD extstyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);

		// Exclude all edge style
		extstyle &= (~WS_EX_CLIENTEDGE) & (~WS_EX_STATICEDGE);
		extstyle |= !get_pseudo_transparent() ? edge_style_from_config(get_edge_style()) : 0;
		SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, extstyle);
		SetWindowPos(m_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	m_watched_handle.release();

	if (get_disabled())
	{
		PostMessage(m_hwnd, UWM_SCRIPT_DISABLE, 0, 0);

		return false;
	}

	HRESULT hr = _script_init();

	if (FAILED(hr))
	{
		// Format error message
		pfc::string_simple win32_error_msg = format_win32_error(hr);
		pfc::string_formatter msg_formatter;

		msg_formatter << "Scripting Engine Initialization Failed (GUID: "
			<< pfc::print_guid(get_config_guid()) << ", CODE: 0x" 
			<< pfc::format_hex_lowercase((unsigned)hr);

		if (hr != E_UNEXPECTED)
		{
			msg_formatter << "): " << win32_error_msg;
		}
		else
		{
			msg_formatter << ")\nCheck the console for more detailed information (Always caused by unexcepted script error).";
		}

		// Show error message
		popup_message::g_show(msg_formatter, "WSH Panel Mod", popup_message::icon_error);
		return false;
	}

	//delete_context();
	//create_context();

	// Show init message
	console::formatter() << "WSH Panel Mod (GUID: " 
		<< pfc::print_guid(get_config_guid()) 
		<< "): initliased in "
		<< timer.query() / 1000
		<< " s";

	// HACK: Script update will not call on_size, so invoke it explicitly
	SendMessage(m_hwnd, UWM_SIZE, 0, 0);

	return true;
}

void uie_win::script_stop()
{
	if (m_script_engine)
	{
		m_script_engine->Close();
	}
}

void uie_win::script_term()
{
	script_stop();

	if (m_script_root)
	{
		m_script_root.Release();
	}

	if (m_script_engine)
	{
		m_script_engine.Release();
	}
}

HRESULT uie_win::script_invoke_v(LPOLESTR name, UINT argc /*= 0*/, VARIANTARG * argv /*= NULL*/, VARIANT * ret /*= NULL*/)
{
	if (!m_script_site.IsAvailable()) return E_NOINTERFACE;
	if (!m_script_root || !m_script_engine) return E_NOINTERFACE;
	if (!name) return E_INVALIDARG;

	DISPID dispid;
	DISPPARAMS param = { argv, NULL, argc, 0 };

	HRESULT hr = m_script_root->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispid);
	
	if (SUCCEEDED(hr))
	{
		IDispatch * pdisp = m_script_root;

		pdisp->AddRef();

		try
		{
			hr = pdisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &param, ret, NULL, NULL);
		}
		catch (std::exception & e)
		{
			pfc::print_guid guid(get_config_guid());

			console::printf("Error: WSH Panel Mod (GUID: %s): %s", guid.get_ptr(), e.what());
		}
		catch (_com_error & e)
		{
			pfc::print_guid guid(get_config_guid());

			console::printf("Error: WSH Panel Mod (GUID: %s): Code: 0x%08x", guid.get_ptr(), e.Error());
		}

		pdisp->Release();
	}

	return hr;
}

void uie_win::create_context()
{
	if (m_gr_bmp || m_gr_bmp_bk)
		delete_context();

	m_gr_bmp = CreateCompatibleBitmap(m_hdc, m_width, m_height);

	if (get_pseudo_transparent())
	{
		m_gr_bmp_bk = CreateCompatibleBitmap(m_hdc, m_width, m_height);
	}
}

void uie_win::delete_context()
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

const GUID& uie_win::get_extension_guid() const
{
	// {75A7B642-786C-4f24-9B52-17D737DEA09A}
	static const GUID ext_guid =
	{ 0x75a7b642, 0x786c, 0x4f24, { 0x9b, 0x52, 0x17, 0xd7, 0x37, 0xde, 0xa0, 0x9a } };

	return ext_guid;
}

void uie_win::get_name(pfc::string_base& out) const
{
	out = "WSH Panel Mod";
}

void uie_win::get_category(pfc::string_base& out) const
{
	out = "Panels";
}

unsigned uie_win::get_type() const
{
	return ui_extension::type_toolbar | ui_extension::type_panel;
}

ui_helpers::container_window::class_data & uie_win::get_class_data() const
{
	static ui_helpers::container_window::class_data my_class_data =
	{
		_T("uie_wsh_panel_mod_class"), 
		_T(""), 
		0, 
		false, 
		false, 
		0, 
		WS_CHILD, 
		!get_pseudo_transparent() ? edge_style_from_config(get_edge_style()) : 0,
		CS_DBLCLKS,
		true, true, true, IDC_ARROW
	};

	return my_class_data;
}

void uie_win::set_config(stream_reader * reader, t_size size, abort_callback & abort)
{
	load_config(reader, size, abort);
}

void uie_win::get_config(stream_writer * writer, abort_callback & abort) const
{
	save_config(writer, abort);
}

void uie_win::get_menu_items(ui_extension::menu_hook_t& hook)
{
	hook.add_node(new menu_node_properties(this));
	hook.add_node(new ui_extension::menu_node_configure(this));
}

bool uie_win::have_config_popup() const
{
	return true;
}

bool uie_win::show_config_popup(HWND parent)
{
	CDialogConf dlg(this);
	return (dlg.DoModal(parent) == IDOK);
}

bool uie_win::show_property_popup(HWND parent)
{
	CDialogProperty dlg(this);
	return (dlg.DoModal(parent) == IDOK);
}

LRESULT uie_win::on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	TRACK_CALL_TEXT_FORMAT("uie_win::on_message(hwnd=0x%x,msg=%u,wp=%d,lp=%d)", m_hwnd, msg, wp, lp);

	switch (msg)
	{
	case WM_CREATE:
		{
			m_hwnd = hwnd;
			m_hdc = GetDC(m_hwnd);
			m_width  = ((CREATESTRUCT*)lp)->cx;
			m_height = ((CREATESTRUCT*)lp)->cy;

			create_context();

			// Interfaces
			m_gr_wrap.Attach(new com_object_impl_t<GdiGraphics>(), false);

			// Callbacks
			static_api_ptr_t<play_callback_manager>()->register_callback(this, play_callback::flag_on_playback_all | play_callback::flag_on_volume_change, true);
			static_api_ptr_t<playlist_manager>()->register_callback(this, playlist_callback::flag_on_playback_order_changed | playlist_callback::flag_on_item_focus_change);
			static_api_ptr_t<metadb_io_v3>()->register_callback(this);

			g_get_panel_notifier().add_window(m_hwnd);

			script_init();
		}
		return 0;

	case WM_DESTROY:
		// Term script
		script_term();

		g_get_panel_notifier().remove_window(m_hwnd);

		// Callbacks
		static_api_ptr_t<play_callback_manager>()->unregister_callback(this);
		static_api_ptr_t<playlist_manager>()->unregister_callback(this);
		static_api_ptr_t<metadb_io_v3>()->unregister_callback(this);

		if (m_gr_wrap)
			m_gr_wrap.Release();

		delete_context();
		ReleaseDC(m_hwnd, m_hdc);
		return 0;

	case WM_DISPLAYCHANGE:
		script_term();
		script_init();
		return 0;

	case WM_ERASEBKGND:
		if (get_pseudo_transparent())
			PostMessage(m_hwnd, UWM_REFRESHBK, 0, 0);

		return 1;

	case UWM_REFRESHBK:
		Redraw();
		return 0;

	case UWM_REFRESHBKDONE:
		script_invoke_v(L"on_refresh_background_done");
		return 0;

	case WM_PAINT:
		{
			if (get_pseudo_transparent() && m_bk_updating)
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
		on_size(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));

		if (get_pseudo_transparent())
			PostMessage(m_hwnd, UWM_REFRESHBK, 0, 0);
		else
			Repaint();

		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		{
			if (get_grab_focus())
				SetFocus(hwnd);

			SetCapture(hwnd);

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
				script_invoke_v(L"on_mouse_lbtn_down", 3, args);
				break;

			case WM_MBUTTONDOWN:
				script_invoke_v(L"on_mouse_mbtn_down", 3, args);
				break;

			case WM_RBUTTONDOWN:
				script_invoke_v(L"on_mouse_rbtn_down", 3, args);
				break;
			}
		}
		break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		{
			ReleaseCapture();

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
				script_invoke_v(L"on_mouse_lbtn_up", 3, args);
				break;

			case WM_MBUTTONUP:
				script_invoke_v(L"on_mouse_mbtn_up", 3, args);
				break;

			case WM_RBUTTONUP:
				{
					_variant_t var_result;

					if (SUCCEEDED(script_invoke_v(L"on_mouse_rbtn_up", 3, args, &var_result)))
					{
						_variant_t var_suppress_menu;

						if (SUCCEEDED(VariantChangeType(&var_suppress_menu, &var_result, 0, VT_BOOL)))
						{
							if ((var_suppress_menu.boolVal != FALSE))
								return 0;
						}
					}
				}
				break;
			}
		}
		break;

	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
		{
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
				script_invoke_v(L"on_mouse_lbtn_dblclk", 3, args);
				break;

			case WM_MBUTTONDBLCLK:
				script_invoke_v(L"on_mouse_mbtn_dblclk", 3, args);
				break;

			case WM_RBUTTONDOWN:
				script_invoke_v(L"on_mouse_rbtn_dblclk", 3, args);
				break;
			}
		}
		break;

	case WM_MOUSEMOVE:
		{
			if (!m_ismousetracked)
			{
				TRACKMOUSEEVENT tme;

				tme.cbSize = sizeof(tme);
				tme.hwndTrack = m_hwnd;
				tme.dwFlags = TME_LEAVE;
				TrackMouseEvent(&tme);
				m_ismousetracked = true;

				// Restore default cursor
				SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
			}

			VARIANTARG args[2];

			args[0].vt = VT_I4;
			args[0].lVal = GET_Y_LPARAM(lp);
			args[1].vt = VT_I4;
			args[1].lVal = GET_X_LPARAM(lp);	
			script_invoke_v(L"on_mouse_move", 2, args);
		}
		break;

	case WM_MOUSELEAVE:
		{
			m_ismousetracked = false;

			script_invoke_v(L"on_mouse_leave");
			// Restore default cursor
			SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
		}
		break;

	case WM_MOUSEWHEEL:
		{
			VARIANTARG args[1];

			args[0].vt = VT_I4;
			args[0].lVal = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
			script_invoke_v(L"on_mouse_wheel", 1, args);
		}
		break;

	case WM_SETCURSOR:
		return true;

	case WM_KEYDOWN:
		{
			static_api_ptr_t<keyboard_shortcut_manager_v2>  ksm;

			if (!ksm->process_keydown_simple(wp))
			{
				VARIANTARG args[1];

				args[0].vt = VT_UI4;
				args[0].ulVal = (ULONG) wp;
				script_invoke_v(L"on_key_down", 1, args);
			}
		}
		break;

	case WM_SETFOCUS:
		{
			VARIANTARG args[1];

			args[0].vt = VT_BOOL;
			args[0].boolVal = VARIANT_TRUE;
			script_invoke_v(L"on_focus", 1, args);
		}
		break;

	case WM_KILLFOCUS:
		{
			VARIANTARG args[1];

			args[0].vt = VT_BOOL;
			args[0].boolVal = VARIANT_FALSE;
			script_invoke_v(L"on_focus", 1, args);
		}
		break;

	case UWM_SCRIPT_ERROR_TIMEOUT:
		get_disabled() = true;
		script_stop();

		popup_message::g_show(pfc::string_formatter() << "Script terminated due to the panel (GUID: " 
			<< pfc::print_guid(get_config_guid())
			<< ") seems to be unresponsive, "
			<< "please check your script (usually infinite loop).", 
			"WSH Panel Mod", popup_message::icon_error);

		Repaint();
		return 0;

	case UWM_SCRIPT_ERROR:
		script_stop();

		if (lp)
		{
			pfc::stringcvt::string_utf8_from_wide utf8((LPCWSTR)lp);

			console::error(utf8);
			MessageBeep(MB_ICONASTERISK);
		}

		Repaint();
		return 0;

	case UWM_SCRIPT_DISABLE:
		// Show error message
		popup_message::g_show(pfc::string_formatter() 
			<< "Panel (GUID: "
			<< pfc::print_guid(get_config_guid()) 
			<< "): Refuse to load script due to critical error last run,"
			<< " please check your script and apply it again.",
			"WSH Panel Mod", 
			popup_message::icon_error);
		return 0;

	case UWM_TIMER:
		on_timer(wp);
		return 0;

	case UWM_PLAYLIST_STOP_AFTER_CURRENT:
		{
			VARIANTARG args[1];

			args[0].vt = VT_BOOL;
			args[0].boolVal = TO_VARIANT_BOOL(wp);
			script_invoke_v(L"on_playlist_stop_after_current_changed", 1, args);
		}
		return 0;

	case UWM_CURSOR_FOLLOW_PLAYBACK:
		{
			VARIANTARG args[1];

			args[0].vt = VT_BOOL;
			args[0].boolVal = TO_VARIANT_BOOL(wp);
			script_invoke_v(L"on_cursor_follow_playback_changed", 1, args);
		}
		return 0;

	case UWM_PLAYBACK_FOLLOW_CURSOR:
		{
			VARIANTARG args[1];

			args[0].vt = VT_BOOL;
			args[0].boolVal = TO_VARIANT_BOOL(wp);
			script_invoke_v(L"on_playback_follow_cursor_changed", 1, args);
		}
		return 0;

	case UWM_NOTIFY_DATA:
		{
			VARIANTARG args[2];

			args[0].vt = VT_BSTR;
			args[0].bstrVal = (BSTR)lp;
			args[1].vt = VT_BSTR;
			args[1].bstrVal = (BSTR)wp;
			script_invoke_v(L"on_notify_data", 2, args);
		}
		return 0;

	case UWM_SHOWCONFIGURE:
		show_config_popup(m_hwnd);
		return 0;

	case UWM_SHOWPROPERTIES:
		show_property_popup(m_hwnd);
		return 0;

	case UWM_GETALBUMARTASYNCDONE:
		{
			using namespace helpers;
			album_art_async::t_param * param = reinterpret_cast<album_art_async::t_param *>(lp);
			VARIANTARG args[3];

			args[0].vt = VT_DISPATCH;
			args[0].pdispVal = param->bitmap;
			args[1].vt = VT_I4;
			args[1].lVal = param->art_id;
			args[2].vt = VT_DISPATCH;
			args[2].pdispVal = param->handle;
			script_invoke_v(L"on_get_album_art_done", 3, args);
			// Release
			delete param;
		}
		return 0;

	case UWM_ON_ITEM_PLAYED:
		{
			metadb_handle * meta = reinterpret_cast<metadb_handle *>(lp);
			FbMetadbHandle * handle = new com_object_impl_t<FbMetadbHandle>(meta);
			VARIANTARG args[1];
			
			args[0].vt = VT_DISPATCH;
			args[0].pdispVal = handle;
			script_invoke_v(L"on_item_played", 1, args);

			handle->Release();
		}
		return 0;

	case UWM_TOGGLEQUERYCONTINUE:
		m_script_site.ToggleQueryContinue(!!wp);
		return 0;

	case UWM_GETCONFIGGUID:
		GUID * guid_ptr = reinterpret_cast<GUID *>(lp);
		memcpy(guid_ptr, &get_config_guid(), sizeof(GUID));
		return 0;
	}

	return uDefWindowProc(hwnd, msg, wp, lp);
}

void uie_win::on_size(int w, int h)
{
	TRACK_FUNCTION();

	m_width = w;
	m_height = h;

	delete_context();
	create_context();

	script_invoke_v(L"on_size");
}

void uie_win::on_paint(HDC dc, LPRECT lpUpdateRect)
{
	TRACK_FUNCTION();

	if (!dc || !lpUpdateRect || !m_gr_bmp || !m_gr_wrap)
		return;

	HDC memdc = CreateCompatibleDC(dc);
	HBITMAP oldbmp = SelectBitmap(memdc, m_gr_bmp);
	
	if (m_script_site.IsAvailable())
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

		Gdiplus::Graphics gr(memdc);
		Gdiplus::Rect rect(lpUpdateRect->left, lpUpdateRect->top,
			lpUpdateRect->right - lpUpdateRect->left,
			lpUpdateRect->bottom - lpUpdateRect->top);

		// SetClip() may improve performance
		gr.SetClip(rect);

		m_gr_wrap->put__ptr(&gr);

		{
			VARIANTARG args[1];

			args[0].vt = VT_DISPATCH;
			args[0].pdispVal = m_gr_wrap;
			script_invoke_v(L"on_paint", 1, args);
		}

		m_gr_wrap->put__ptr(NULL);
	}
	else
	{
		const TCHAR szErrMsg[] = _T("SCRIPT ERROR");
		RECT rc = {0, 0, m_width, m_height};

		FillRect(memdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
		SetBkMode(memdc, TRANSPARENT);
		SetTextColor(memdc, GetSysColor(COLOR_WINDOWTEXT));
		DrawText(memdc, szErrMsg, -1, &rc, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE);
	}

	BitBlt(dc, 0, 0, m_width, m_height, memdc, 0, 0, SRCCOPY);
	SelectBitmap(memdc, oldbmp);
	DeleteDC(memdc);
}

void uie_win::on_timer(UINT timer_id)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_UI4;
	args[0].uintVal = timer_id;
	script_invoke_v(L"on_timer", 1, args);
}

void uie_win::on_playback_starting(play_control::t_track_command cmd, bool paused)
{
	TRACK_FUNCTION();

	VARIANTARG args[2];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(paused);
	args[1].vt = VT_I4;
	args[1].lVal = cmd;
	script_invoke_v(L"on_playback_starting", 2, args);
}

void uie_win::on_playback_new_track(metadb_handle_ptr track)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];
	FbMetadbHandle * handle = new com_object_impl_t<FbMetadbHandle>(track);
	
	args[0].vt = VT_DISPATCH;
	args[0].pdispVal = handle;
	script_invoke_v(L"on_playback_new_track", 1, args);

	handle->Release();
}

void uie_win::on_playback_stop(play_control::t_stop_reason reason)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_I4;
	args[0].lVal = reason;
	script_invoke_v(L"on_playback_stop", 1, args);
}

void uie_win::on_playback_seek(double time)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_R8;
	args[0].dblVal = time;
	script_invoke_v(L"on_playback_seek", 1, args);
}

void uie_win::on_playback_pause(bool state)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(state);
	script_invoke_v(L"on_playback_pause", 1, args);
}

void uie_win::on_playback_edited(metadb_handle_ptr track)
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_playback_edited");
}

void uie_win::on_playback_dynamic_info(const file_info& info)
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_playback_dynamic_info");
}

void uie_win::on_playback_dynamic_info_track(const file_info& info)
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_playback_dynamic_info_track");
}

void uie_win::on_playback_time(double time)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_R8;
	args[0].dblVal = time;
	script_invoke_v(L"on_playback_time", 1, args);
}

void uie_win::on_volume_change(float newval)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_R4;
	args[0].fltVal = newval;
	script_invoke_v(L"on_volume_change", 1, args);
}

void uie_win::on_playback_order_changed(t_size p_new_index)
{
	TRACK_FUNCTION();

	VARIANTARG args[1];

	args[0].vt = VT_I4;
	args[0].lVal = p_new_index;
	script_invoke_v(L"on_playback_order_changed", 1, args);
}

void uie_win::on_item_focus_change(t_size p_playlist,t_size p_from,t_size p_to)
{
	TRACK_FUNCTION();

	script_invoke_v(L"on_item_focus_change");
}

void uie_win::on_changed_sorted(metadb_handle_list_cref p_items_sorted, bool p_fromhook)
{
	TRACK_FUNCTION();

	if (m_watched_handle.is_empty() || !p_items_sorted.have_item(m_watched_handle))
		return;

	VARIANTARG args[2];
	FbMetadbHandle * handle = new com_object_impl_t<FbMetadbHandle>(m_watched_handle);

	args[0].vt = VT_BOOL;
	args[0].boolVal = TO_VARIANT_BOOL(p_fromhook);
	args[1].vt = VT_DISPATCH;
	args[1].pdispVal = handle;
	script_invoke_v(L"on_metadb_changed", 2, args);

	handle->Release();
}
