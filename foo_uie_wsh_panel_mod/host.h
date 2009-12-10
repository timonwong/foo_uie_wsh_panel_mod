#pragma once


#include "script_interface_impl.h"
#include "config.h"

_COM_SMARTPTR_TYPEDEF(IActiveScriptParse, IID_IActiveScriptParse);


class HostComm : public wsh_panel_vars
{
protected:
	HWND              m_hwnd;
	UINT              m_width;
	UINT              m_height;
	POINT             m_max_size;
	POINT             m_min_size;
	HDC               m_hdc;
	HBITMAP           m_gr_bmp;
	HBITMAP           m_gr_bmp_bk;
	bool              m_bk_updating;
	bool              m_paint_pending;
	UINT              m_accuracy;
	metadb_handle_ptr m_watched_handle;

	IActiveScriptPtr        m_script_engine;
	IDispatchPtr            m_script_root;
	SCRIPTSTATE             m_sstate;
	bool                    m_query_continue;

	HostComm();
	virtual ~HostComm();

public:
	GUID GetGUID();
	HWND GetHWND();
	UINT GetWidth();
	UINT GetHeight();
	POINT & GetMaxSize();
	POINT & GetMinSize();
	SCRIPTSTATE & GetScriptState();
	metadb_handle_ptr & GetWatchedMetadbHandle();
	IGdiBitmap * GetBackgroundImage();
	bool & GetQueryContinue();

	void Redraw();
	void Repaint(bool force = false);
	void RepaintRect(UINT x, UINT y, UINT w, UINT h, bool force = false);

	void OnScriptError(LPCWSTR str);
	void RefreshBackground(LPRECT lprcUpdate = NULL);

	ITimerObj * CreateTimerTimeout(UINT timeout);
	ITimerObj * CreateTimerInterval(UINT delay);
	void KillTimer(ITimerObj * p);

	static void CALLBACK g_timer_proc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
};

class FbWindow : public IDispatchImpl3<IFbWindow>
{
private:
	HostComm * m_host;

protected:
	FbWindow(HostComm* p) : m_host(p) {}
	virtual ~FbWindow() {}

public:
	STDMETHODIMP get_ID(UINT* p);
	STDMETHODIMP get_Width(UINT* p);
	STDMETHODIMP get_Height(UINT* p);
	STDMETHODIMP get_MaxWidth(UINT* p);
	STDMETHODIMP put_MaxWidth(UINT width);
	STDMETHODIMP get_MaxHeight(UINT* p);
	STDMETHODIMP put_MaxHeight(UINT height);
	STDMETHODIMP get_MinWidth(UINT* p);
	STDMETHODIMP put_MinWidth(UINT width);
	STDMETHODIMP get_MinHeight(UINT* p);
	STDMETHODIMP put_MinHeight(UINT height);
	STDMETHODIMP Repaint(VARIANT_BOOL force);
	STDMETHODIMP RepaintRect(UINT x, UINT y, UINT w, UINT h, VARIANT_BOOL force);
	STDMETHODIMP CreatePopupMenu(IMenuObj ** pp);
	STDMETHODIMP CreateTimerTimeout(UINT timeout, ITimerObj ** pp);
	STDMETHODIMP CreateTimerInterval(UINT delay, ITimerObj ** pp);
	STDMETHODIMP KillTimer(ITimerObj * p);
	STDMETHODIMP NotifyOthers(BSTR name, BSTR info);
	STDMETHODIMP WatchMetadb(IFbMetadbHandle * handle);
	STDMETHODIMP UnWatchMetadb();
	STDMETHODIMP CreateTooltip(IFbTooltip ** pp);
	STDMETHODIMP ShowConfigure();
	STDMETHODIMP ShowProperties();
	STDMETHODIMP GetProperty(BSTR name, VARIANT defaultval, VARIANT * p);
	STDMETHODIMP SetProperty(BSTR name, VARIANT val);
	STDMETHODIMP GetBackgroundImage(IGdiBitmap ** pp);
	STDMETHODIMP SetCursor(UINT id);
};

class ScriptSite : 
	public IActiveScriptSite,
	public IActiveScriptSiteWindow,
	public IActiveScriptSiteInterruptPoll
{
private:
	HostComm*    m_host;
	IFbWindowPtr m_window;
	IGdiUtilsPtr m_gdi;
	IFbUtilsPtr  m_fb2k;
	IWSHUtilsPtr m_utils;
	DWORD        m_dwRef;
	DWORD        m_dwStartTime;

	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IActiveScriptSite)
		COM_QI_ENTRY(IActiveScriptSite)
		COM_QI_ENTRY(IActiveScriptSiteWindow)
		COM_QI_ENTRY(IActiveScriptSiteInterruptPoll)
	END_COM_QI_IMPL()

public:
	ScriptSite(HostComm * p_host)
		: m_host(p_host)
		, m_window(new com_object_impl_t<FbWindow, false>(p_host))
		, m_gdi(new com_object_impl_t<GdiUtils, false>())
		, m_fb2k(new com_object_impl_t<FbUtils, false>())
		, m_utils(new com_object_impl_t<WSHUtils, false>())
		, m_dwRef(0)
		, m_dwStartTime(0)
	{}

	virtual ~ScriptSite()
	{
	}

public:
	// IUnknown
	STDMETHODIMP_(ULONG) AddRef()
	{
		return ++m_dwRef;
	}

	STDMETHODIMP_(ULONG) Release()
	{
		return --m_dwRef; 
	}

	// IActiveScriptSite
	STDMETHODIMP GetLCID(LCID* plcid);
	STDMETHODIMP GetItemInfo(LPCOLESTR name, DWORD mask, IUnknown** ppunk, ITypeInfo** ppti);
	STDMETHODIMP GetDocVersionString(BSTR* pstr);
	STDMETHODIMP OnScriptTerminate(const VARIANT* result, const EXCEPINFO* excep);
	STDMETHODIMP OnStateChange(SCRIPTSTATE state);
	STDMETHODIMP OnScriptError(IActiveScriptError* err);
	STDMETHODIMP OnEnterScript();
	STDMETHODIMP OnLeaveScript();

	// IActiveScriptSiteWindow
	STDMETHODIMP GetWindow(HWND *phwnd);
	STDMETHODIMP EnableModeless(BOOL fEnable);

	// IActiveScriptSiteInterruptPoll
	STDMETHODIMP QueryContinue();
};

class CDialogConf;
class CDialogProperty;

class wsh_panel_window : 
	public HostComm, 
	public ui_extension::container_ui_extension
{
private:
	// Scripting
	ScriptSite       m_script_site;
	IGdiGraphicsPtr  m_gr_wrap;
	bool             m_ismousetracked;

public:
	virtual ~wsh_panel_window()
	{
		// Ensure active scripting is closed
		script_term();
	}

	wsh_panel_window();

	void on_update_script(const char* name, const char* code);
	HRESULT _script_init();
	bool script_init();
	void script_stop();
	void script_term();
	HRESULT script_invoke_v(LPOLESTR name, UINT argc = 0, VARIANTARG * argv = NULL, VARIANT * ret = NULL);
	void create_context();
	void delete_context();

	// ui_extension
	const GUID & get_extension_guid() const;
	void get_name(pfc::string_base& out) const;
	void get_category(pfc::string_base& out) const;
	unsigned get_type() const;
	class_data & get_class_data() const;
	void set_config(stream_reader * reader, t_size size, abort_callback & abort);
	void get_config(stream_writer * writer, abort_callback & abort) const;
	void get_menu_items(ui_extension::menu_hook_t& hook);
	bool have_config_popup() const;
	bool show_config_popup(HWND parent);
	LRESULT on_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

	bool show_property_popup(HWND parent);
	void on_size(int w, int h);
	void on_paint(HDC dc, LPRECT lpUpdateRect);
	void on_timer(UINT timer_id);

	// callbacks
	void on_get_album_art_done(LPARAM lp);
	void on_item_played(WPARAM wp);
	void on_playlist_stop_after_current_changed(WPARAM wp);
	void on_cursor_follow_playback_changed(WPARAM wp);
	void on_playback_follow_cursor_changed(WPARAM wp);
	void on_notify_data(WPARAM wp);

	// play_callback
	void on_playback_starting(play_control::t_track_command cmd, bool paused);
	void on_playback_new_track(WPARAM wp);
	void on_playback_stop(play_control::t_stop_reason reason);
	void on_playback_seek(WPARAM wp);
	void on_playback_pause(bool state);
	void on_playback_edited();
	void on_playback_dynamic_info();
	void on_playback_dynamic_info_track();
	void on_playback_time(WPARAM wp);
	void on_volume_change(WPARAM wp);

	// playlist_callback
	void on_item_focus_change();
	void on_playback_order_changed(t_size p_new_index);

	// metadb_io_callback_dynamic
	void on_changed_sorted(WPARAM wp);
};
