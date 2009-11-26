#pragma once


#include "script_interface_impl.h"
#include "config.h"


class HostComm : public wsh_panel_vars
{
protected:
	HWND m_hwnd;
	UINT m_width;
	UINT m_height;
	POINT m_max_size;
	POINT m_min_size;
	HDC  m_hdc;
	HBITMAP  m_gr_bmp;
	HBITMAP  m_gr_bmp_bk;
	bool m_bk_updating;
	bool m_paint_pending;
	UINT m_accuracy;
	metadb_handle_ptr m_watched_handle;

	HostComm();
	virtual ~HostComm();

public:
	GUID GetGUID();
	HWND GetHWND();
	UINT GetWidth();
	UINT GetHeight();
	POINT & GetMaxSize();
	POINT & GetMinSize();
	void Redraw();
	void Repaint(bool force = false);
	void RepaintRect(UINT x, UINT y, UINT w, UINT h, bool force = false);
	void OnScriptError(LPCWSTR str);
	void RefreshBackground(LPRECT lprcUpdate = NULL);
	ITimerObj * CreateTimerTimeout(UINT timeout);
	ITimerObj * CreateTimerInterval(UINT delay);
	void KillTimer(ITimerObj * p);
	metadb_handle_ptr & GetWatchedMetadbHandle();
	IGdiBitmap * GetBackgroundImage();
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

class ScriptSite : public IActiveScriptSite,
	public IActiveScriptSiteWindow,
	public IActiveScriptSiteInterruptPoll
{
private:
	HostComm*    m_host;
	IFbWindowPtr m_window;
	IGdiUtilsPtr m_gdi;
	IFbUtilsPtr  m_fb2k;
	IWSHUtilsPtr m_utils;
	SCRIPTSTATE  m_sstate;
	DWORD        m_dwRef;
	DWORD        m_dwStartTime;
	bool         m_bQueryContinue;

	BEGIN_COM_QI_IMPL()
		COM_QI_INTERFACE_ENTRY(IActiveScriptSite)
		COM_QI_INTERFACE_ENTRY(IActiveScriptSiteWindow)
		COM_QI_INTERFACE_ENTRY(IActiveScriptSiteInterruptPoll)
	END_COM_QI_IMPL()

public:
	ScriptSite(HostComm * p_host)
		: m_host(p_host)
		, m_window(new com_object_impl_t<FbWindow, false>(p_host))
		, m_gdi(new com_object_impl_t<GdiUtils, false>())
		, m_fb2k(new com_object_impl_t<FbUtils, false>())
		, m_utils(new com_object_impl_t<WSHUtils, false>())
		, m_sstate(SCRIPTSTATE_UNINITIALIZED)
		, m_dwRef(0)
		, m_dwStartTime(0)
	{}

	virtual ~ScriptSite()
	{
	}

public:
	bool IsAvailable() const
	{
		return (m_sstate == SCRIPTSTATE_CONNECTED);
	}

	void ToggleQueryContinue(bool bToggle)
	{
		m_bQueryContinue = bToggle;
	}

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

class uie_win : 
	public HostComm, 
	public ui_extension::container_ui_extension, 
	public play_callback,
	public playlist_callback,
	public metadb_io_callback_dynamic
{
private:
	// Scripting
	IActiveScriptPtr m_script_engine;
	IDispatchPtr     m_script_root;
	ScriptSite       m_script_site;

	IGdiGraphicsPtr  m_gr_wrap;
	bool m_ismousetracked;

public:
	virtual ~uie_win()
	{
		// Ensure active scripting is closed
		script_term();
	}

	uie_win();

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

	// playback_callback
	void on_playback_starting(play_control::t_track_command cmd, bool paused);
	void on_playback_new_track(metadb_handle_ptr track);
	void on_playback_stop(play_control::t_stop_reason reason);
	void on_playback_seek(double time);
	void on_playback_pause(bool state);
	void on_playback_edited(metadb_handle_ptr track);
	void on_playback_dynamic_info(const file_info& info);
	void on_playback_dynamic_info_track(const file_info& info);
	void on_playback_time(double time);
	void on_volume_change(float newval);

	// playlist_callback
	void on_items_added(t_size p_playlist,t_size p_start, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection){}
	void on_items_reordered(t_size p_playlist,const t_size * p_order,t_size p_count){}
	void on_items_removing(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count){}
	void on_items_removed(t_size p_playlist,const bit_array & p_mask,t_size p_old_count,t_size p_new_count){}
	void on_items_selection_change(t_size p_playlist,const bit_array & p_affected,const bit_array & p_state){}
	void on_item_focus_change(t_size p_playlist,t_size p_from,t_size p_to);
	void on_items_modified(t_size p_playlist,const bit_array & p_mask){}
	void on_items_modified_fromplayback(t_size p_playlist,const bit_array & p_mask,play_control::t_display_level p_level){}
	void on_items_replaced(t_size p_playlist,const bit_array & p_mask,const pfc::list_base_const_t<t_on_items_replaced_entry> & p_data){}
	void on_item_ensure_visible(t_size p_playlist,t_size p_idx){}
	void on_playlist_activate(t_size p_old,t_size p_new){}
	void on_playlist_created(t_size p_index,const char * p_name,t_size p_name_len){}
	void on_playlists_reorder(const t_size * p_order,t_size p_count){}
	void on_playlists_removing(const bit_array & p_mask,t_size p_old_count,t_size p_new_count){}
	void on_playlists_removed(const bit_array & p_mask,t_size p_old_count,t_size p_new_count){}
	void on_playlist_renamed(t_size p_index,const char * p_new_name,t_size p_new_name_len){}
	void on_default_format_changed(){}
	void on_playback_order_changed(t_size p_new_index);
	void on_playlist_locked(t_size p_playlist,bool p_locked){}

	// metadb_io_callback_dynamic
	void on_changed_sorted(metadb_handle_list_cref p_items_sorted, bool p_fromhook);
};
