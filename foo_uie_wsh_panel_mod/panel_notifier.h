#pragma once


class panel_notifier_manager
{
public:
	typedef pfc::chain_list_v2_t<HWND> t_hwndlist;

	panel_notifier_manager()
	{
	}

	static inline panel_notifier_manager & instance()
	{
		return sm_instance;
	}

	inline void add_window(HWND p_wnd)
	{
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

	void send_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp);
	void send_msg_to_others_pointer(HWND p_wnd_except, UINT p_msg, pfc::refcounted_object_root * p_param);
	//void post_msg_to_others_pointer(HWND p_wnd_except, UINT p_msg, pfc::refcounted_object_root * p_param);
	void post_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp);
	void post_msg_to_all_pointer(UINT p_msg, pfc::refcounted_object_root * p_param);

private:
	t_hwndlist m_hwnds;
	static panel_notifier_manager sm_instance;

	PFC_CLASS_NOT_COPYABLE_EX(panel_notifier_manager)
};

class config_object_callback : public config_object_notify
{
public:
	virtual t_size get_watched_object_count();
	virtual GUID get_watched_object(t_size p_index);
	virtual void on_watched_object_changed(const service_ptr_t<config_object> & p_object);
};

template <class T>
struct t_simple_callback_data : public pfc::refcounted_object_root
{
	T m_item;

	inline t_simple_callback_data(const T & p_item) : m_item(p_item) {}
};

template <class T1, class T2>
struct t_simple_callback_data_2 : public pfc::refcounted_object_root
{
	T1 m_item1;
	T2 m_item2;

	inline t_simple_callback_data_2(const T1 & p_item1, const T2 & p_item2) : m_item1(p_item1), m_item2(p_item2) {}
};

// Only used in message handler
template <class T>
class callback_data_ptr
{
private:
	T * m_data;

public:
	template <class TParam>
	inline callback_data_ptr(TParam p_data)
	{
		m_data = reinterpret_cast<T *>(p_data);
	}

	template <class TParam>
	inline callback_data_ptr(TParam * p_data)
	{
		m_data = reinterpret_cast<T *>(p_data);
	}

	inline virtual ~callback_data_ptr()
	{
		m_data->refcount_release();
	}

	T * operator->()
	{
		return m_data;
	}
};

class playback_stat_callback : public playback_statistics_collector
{
public:
	virtual void on_item_played(metadb_handle_ptr p_item);
};

class nonautoregister_callbacks : public initquit, public metadb_io_callback_dynamic, public ui_selection_callback
{
public:
	struct t_on_changed_sorted_data : public pfc::refcounted_object_root
	{
		metadb_handle_list m_items_sorted;
		bool m_fromhook;

		t_on_changed_sorted_data(metadb_handle_list_cref p_items_sorted, bool p_fromhook) 
			: m_items_sorted(p_items_sorted)
			, m_fromhook(p_fromhook)
		{}
	};

	// initquit
	virtual void on_init()
	{
		static_api_ptr_t<metadb_io_v3>()->register_callback(this);
		static_api_ptr_t<ui_selection_manager_v2>()->register_callback(this, 0);
	}

	virtual void on_quit()
	{
		static_api_ptr_t<ui_selection_manager_v2>()->unregister_callback(this);
		static_api_ptr_t<metadb_io_v3>()->unregister_callback(this);
	}

	// metadb_io_callback_dynamic
	virtual void on_changed_sorted(metadb_handle_list_cref p_items_sorted, bool p_fromhook);

	// ui_selection_callback
	virtual void on_selection_changed(metadb_handle_list_cref p_selection);
};

class my_play_callback : public play_callback_static 
{
public:
	// flag_on_playback_all dosen't contain flag_on_volume_change!
	virtual unsigned get_flags() { return flag_on_playback_all | flag_on_volume_change; }

	virtual void on_playback_starting(play_control::t_track_command cmd, bool paused);
	virtual void on_playback_new_track(metadb_handle_ptr track);
	virtual void on_playback_stop(play_control::t_stop_reason reason);
	virtual void on_playback_seek(double time);
	virtual void on_playback_pause(bool state);
	virtual void on_playback_edited(metadb_handle_ptr track);
	virtual void on_playback_dynamic_info(const file_info& info);
	virtual void on_playback_dynamic_info_track(const file_info& info);
	virtual void on_playback_time(double time);
	virtual void on_volume_change(float newval);
};

class my_playlist_callback : public playlist_callback_single_static
{
public:
	virtual unsigned get_flags() { return flag_on_item_focus_change | flag_on_playback_order_changed; }

	virtual void on_items_added(t_size p_base, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const bit_array & p_selection) {}
	virtual void on_items_reordered(const t_size * p_order,t_size p_count) {}
	virtual void on_items_removing(const bit_array & p_mask,t_size p_old_count,t_size p_new_count) {}
	virtual void on_items_removed(const bit_array & p_mask,t_size p_old_count,t_size p_new_count) {}
	virtual void on_items_selection_change(const bit_array & p_affected,const bit_array & p_state) {}
	// impl
	virtual void on_item_focus_change(t_size p_from,t_size p_to);
	virtual void on_items_modified(const bit_array & p_mask) {}
	virtual void on_items_modified_fromplayback(const bit_array & p_mask,play_control::t_display_level p_level) {}
	virtual void on_items_replaced(const bit_array & p_mask,const pfc::list_base_const_t<playlist_callback::t_on_items_replaced_entry> & p_data) {}
	virtual void on_item_ensure_visible(t_size p_idx) {}
	// impl
	virtual void on_playlist_switch();
	virtual void on_playlist_renamed(const char * p_new_name,t_size p_new_name_len) {}
	virtual void on_playlist_locked(bool p_locked) {}

	virtual void on_default_format_changed() {}
	// impl
	virtual void on_playback_order_changed(t_size p_new_index);
};
