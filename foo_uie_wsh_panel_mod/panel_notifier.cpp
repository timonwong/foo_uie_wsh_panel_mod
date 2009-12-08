#include "stdafx.h"
#include "panel_notifier.h"
#include "helpers.h"
#include "user_message.h"


/*static*/ panel_notifier_manager panel_notifier_manager::sm_instance;
static service_factory_single_t<config_object_callback> g_config_object_callback;
static playback_statistics_collector_factory_t<playback_stat_callback> g_stat_collector_callback;
static play_callback_static_factory_t<my_play_callback > g_my_play_callback;
static service_factory_single_t<my_playlist_callback> g_my_playlist_callback;
static initquit_factory_t<metadb_changed_callback> g_metadb_changed_callback;

void panel_notifier_manager::post_msg_to_others_callback(HWND p_wnd_except, UINT p_msg, WPARAM p_wp, LPARAM p_lp, panel_notifier_callback * p_callback)
{
	t_size count = m_hwnds.get_count();

	if (count < 2)
		return;

	panel_notifier_data * data_ptr = new panel_notifier_data(count - 1, p_callback);

	for (t_size i = 0; i < count; ++i)
	{
		HWND wnd = m_hwnds[i];

		if (wnd != p_wnd_except)
		{
			SendMessageCallback(wnd, p_msg, p_wp, p_lp, g_notify_others_callback, (ULONG_PTR)data_ptr);
		}
	}
}

void panel_notifier_manager::send_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp)
{
	for (t_size i = 0; i < m_hwnds.get_count(); ++i)
	{
		HWND wnd = m_hwnds[i];

		SendMessage(wnd, p_msg, p_wp, p_lp);
	}
}

void panel_notifier_manager::post_msg_to_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp)
{
	for (t_size i = 0; i < m_hwnds.get_count(); ++i)
	{
		HWND wnd = m_hwnds[i];

		PostMessage(wnd, p_msg, p_wp, p_lp);
	}
}

void panel_notifier_manager::post_msg_to_all_callback(UINT p_msg, WPARAM p_wp, LPARAM p_lp, panel_notifier_callback * p_callback)
{
	t_size count = m_hwnds.get_count();

	if (count < 1)
		return;

	panel_notifier_data * data_ptr = new panel_notifier_data(count, p_callback);

	for (t_size i = 0; i < count; ++i)
	{
		HWND wnd = m_hwnds[i];

		SendMessageCallback(wnd, p_msg, p_wp, p_lp, g_notify_others_callback, (ULONG_PTR)data_ptr);
	}
}

void CALLBACK panel_notifier_manager::g_notify_others_callback(HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lResult)
{
	panel_notifier_data * data_ptr = reinterpret_cast<panel_notifier_data *>(dwData);
	LONG ref = InterlockedDecrement(&data_ptr->m_ref);

	if (data_ptr)
	{
		data_ptr->m_callback->on_callback(hwnd, uMsg, lResult);

		if (ref == 0)
		{
			delete data_ptr;
		}
	}
}

// shame
template <class T>
struct simple_data_callback : public panel_notifier_callback
{
	T m_param_holder;

	simple_data_callback(const T & param) : m_param_holder(param) {}
	virtual ~simple_data_callback() {}
};

// shame
template <class T>
struct simple_ref_data_callback : public panel_notifier_callback
{
	T m_param_holder;

	simple_ref_data_callback(T param) : m_param_holder(param) {}
	virtual ~simple_ref_data_callback() {}

	virtual void on_callback(HWND hwnd, UINT uMsg, LRESULT lResult) {}
};

t_size config_object_callback::get_watched_object_count()
{
	return 3;
}

GUID config_object_callback::get_watched_object(t_size p_index)
{
	switch (p_index)
	{
	case 0:
		return standard_config_objects::bool_playlist_stop_after_current;

	case 1:
		return standard_config_objects::bool_cursor_follows_playback;

	case 2:
		return standard_config_objects::bool_playback_follows_cursor;
	}

	return pfc::guid_null;
}

void config_object_callback::on_watched_object_changed(const service_ptr_t<config_object> & p_object)
{
	GUID guid = p_object->get_guid();
	bool boolval = false;
	unsigned msg = 0;

	p_object->get_data_bool(boolval);

	if (guid == standard_config_objects::bool_playlist_stop_after_current)
		msg = CALLBACK_UWM_PLAYLIST_STOP_AFTER_CURRENT;
	else if (guid == standard_config_objects::bool_cursor_follows_playback)
		msg = CALLBACK_UWM_CURSOR_FOLLOW_PLAYBACK;
	else
		msg = CALLBACK_UWM_PLAYBACK_FOLLOW_CURSOR;

	panel_notifier_manager::instance().post_msg_to_all(msg, TO_VARIANT_BOOL(boolval), 0);
}

void playback_stat_callback::on_item_played(metadb_handle_ptr p_item)
{
	typedef simple_data_callback<metadb_handle_ptr> callback_t;

	callback_t * p_callback = new callback_t(p_item);

	panel_notifier_manager::instance().post_msg_to_all_callback(CALLBACK_UWM_ON_ITEM_PLAYED, 
		(WPARAM)&p_callback->m_param_holder, 0, p_callback);
}

void metadb_changed_callback::on_changed_sorted(metadb_handle_list_cref p_items_sorted, bool p_fromhook)
{
	typedef simple_ref_data_callback<metadb_handle_list> callback_t;

	callback_t * p_callback = new callback_t(p_items_sorted);

	panel_notifier_manager::instance().post_msg_to_all_callback(CALLBACK_UWM_ON_CHANGED_SORTED, 
		(WPARAM)&p_callback->m_param_holder, (LPARAM)p_fromhook, p_callback);
}

void metadb_changed_callback::on_init()
{
	static_api_ptr_t<metadb_io_v3> io;
	io->register_callback(this);
}

void metadb_changed_callback::on_quit()
{
	static_api_ptr_t<metadb_io_v3> io;
	io->unregister_callback(this);
}

void my_play_callback::on_playback_starting(play_control::t_track_command cmd, bool paused)
{
	panel_notifier_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_STARTING, 
		(WPARAM)cmd, (LPARAM)paused);
}

void my_play_callback::on_playback_new_track(metadb_handle_ptr track)
{
	typedef simple_data_callback<metadb_handle_ptr> callback_t;

	callback_t * p_callback = new callback_t(track);

	panel_notifier_manager::instance().post_msg_to_all_callback(CALLBACK_UWM_ON_PLAYBACK_NEW_TRACK, 
		(WPARAM)&p_callback->m_param_holder, 0, p_callback);
}

void my_play_callback::on_playback_stop(play_control::t_stop_reason reason)
{
	panel_notifier_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_STOP, 
		(WPARAM)reason, 0);
}

void my_play_callback::on_playback_seek(double time)
{
	// sizeof(double) >= sizeof(WPARAM)
	typedef simple_data_callback<double> callback_t;

	callback_t * p_callback = new callback_t(time);

	panel_notifier_manager::instance().post_msg_to_all_callback(CALLBACK_UWM_ON_PLAYBACK_SEEK,
		(WPARAM)&p_callback->m_param_holder, 0, p_callback);
}

void my_play_callback::on_playback_pause(bool state)
{
	panel_notifier_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_PAUSE,
		(WPARAM)state, 0);
}

void my_play_callback::on_playback_edited(metadb_handle_ptr track)
{
	panel_notifier_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_EDITED, 0, 0);
}

void my_play_callback::on_playback_dynamic_info(const file_info& info)
{
	panel_notifier_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO, 0, 0);
}

void my_play_callback::on_playback_dynamic_info_track(const file_info& info)
{
	panel_notifier_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO_TRACK, 0, 0);
}

void my_play_callback::on_playback_time(double time)
{
	// sizeof(double) >= sizeof(WPARAM)
	typedef simple_data_callback<double> callback_t;

	callback_t * p_callback = new callback_t(time);

	panel_notifier_manager::instance().post_msg_to_all_callback(CALLBACK_UWM_ON_PLAYBACK_TIME,
		(WPARAM)&p_callback->m_param_holder, 0, p_callback);
}

void my_play_callback::on_volume_change(float newval)
{
	PFC_STATIC_ASSERT(sizeof(float) == sizeof(WPARAM));

	panel_notifier_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_VOLUME_CHANGE,
		(WPARAM)newval, 0);
}

void my_playlist_callback::on_item_focus_change(t_size p_from,t_size p_to)
{
	panel_notifier_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_ITEM_FOCUS_CHANGE, 0, 0);
}

void my_playlist_callback::on_playback_order_changed(t_size p_new_index)
{
	panel_notifier_manager::instance().post_msg_to_all(CALLBACK_UWM_ON_PLAYBACK_ORDER_CHANGED, 
		(WPARAM)p_new_index, 0);
}
