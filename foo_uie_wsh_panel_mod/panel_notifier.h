#pragma once


class NOVTABLE panel_notifier_callback
{
public:
	virtual void on_callback(HWND hwnd, UINT uMsg, LRESULT lResult) = 0;
};

class panel_notifier
{
public:
	typedef pfc::list_t<HWND> t_hwndlist;

private:
	t_hwndlist m_hwnds;

protected:
	struct panel_notifier_data
	{
	private:
		volatile LONG m_ref;
		panel_notifier_callback * m_callback;

		panel_notifier_data(LONG ref, panel_notifier_callback * p_callback) : m_ref(ref), 
			m_callback(p_callback) { }
		virtual ~panel_notifier_data() { delete m_callback; }

		friend class panel_notifier;
	};

	static void CALLBACK g_notify_others_callback(HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lResult);

public:
	inline void add_window(HWND p_wnd)
	{
		if (m_hwnds.find_item(p_wnd))
			m_hwnds.remove_item(p_wnd);

		m_hwnds.add_item(p_wnd);
	}

	inline void remove_window(HWND p_wnd)
	{
		m_hwnds.remove_item(p_wnd);
	}

	inline t_hwndlist & get_hwnd_list()
	{
		return m_hwnds;
	}

	inline t_size get_count()
	{
		return m_hwnds.get_count();
	}

	// async
	void notify_others_callback(HWND p_wnd_except, UINT p_msg, WPARAM p_wp, LPARAM p_lp, panel_notifier_callback * p_callback);

	void notify_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp);

	void notify_all_async(UINT p_msg, WPARAM p_wp, LPARAM p_lp);

	// async
	void notify_all_callback(UINT p_msg, WPARAM p_wp, LPARAM p_lp, panel_notifier_callback * p_callback);
};

class config_object_notifier : public config_object_notify
{
public:
	virtual t_size get_watched_object_count();
	virtual GUID get_watched_object(t_size p_index);
	virtual void on_watched_object_changed(const service_ptr_t<config_object> & p_object);
};

class stat_collector_notifier : public playback_statistics_collector
{
private:
	struct notify_callback : public panel_notifier_callback
	{
		metadb_handle_ptr m_handle;

		notify_callback(const metadb_handle_ptr & p_handle) : m_handle(p_handle) {}
		virtual ~notify_callback() {}

		virtual void on_callback(HWND hwnd, UINT uMsg, LRESULT lResult) {}
	};

public:
	virtual void on_item_played(metadb_handle_ptr p_item);
};

panel_notifier & g_get_panel_notifier();
