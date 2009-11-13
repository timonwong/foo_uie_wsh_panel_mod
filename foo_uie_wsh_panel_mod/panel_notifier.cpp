#include "stdafx.h"
#include "panel_notifier.h"
#include "helpers.h"
#include "user_message.h"


static service_factory_single_t<config_object_notifier> g_config_object_notifier;
static playback_statistics_collector_factory_t<stat_collector_notifier> g_stat_collector_notifier;
static panel_notifier g_panel_notifier;

panel_notifier & g_get_panel_notifier()
{
	return g_panel_notifier;
}

void panel_notifier::notify_others_callback(HWND p_wnd_except, UINT p_msg, WPARAM p_wp, LPARAM p_lp, panel_notifier_callback * p_callback)
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

void panel_notifier::notify_all(UINT p_msg, WPARAM p_wp, LPARAM p_lp)
{
	for (t_size i = 0; i < m_hwnds.get_count(); ++i)
	{
		HWND wnd = m_hwnds[i];

		SendMessage(wnd, p_msg, p_wp, p_lp);
	}
}

void panel_notifier::notify_all_async(UINT p_msg, WPARAM p_wp, LPARAM p_lp)
{
	for (t_size i = 0; i < m_hwnds.get_count(); ++i)
	{
		HWND wnd = m_hwnds[i];

		PostMessage(wnd, p_msg, p_wp, p_lp);
	}
}

void panel_notifier::notify_all_callback(UINT p_msg, WPARAM p_wp, LPARAM p_lp, panel_notifier_callback * p_callback)
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

void CALLBACK panel_notifier::g_notify_others_callback(HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lResult)
{
	panel_notifier_data * data_ptr = reinterpret_cast<panel_notifier_data *>(dwData);
	LONG ref = InterlockedDecrement(&data_ptr->m_ref);

	data_ptr->m_callback->on_callback(hwnd, uMsg, lResult);

	if (ref == 0)
	{
		delete data_ptr;
	}
}

t_size config_object_notifier::get_watched_object_count()
{
	return 3;
}

GUID config_object_notifier::get_watched_object(t_size p_index)
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

void config_object_notifier::on_watched_object_changed(const service_ptr_t<config_object> & p_object)
{
	GUID guid = p_object->get_guid();
	bool boolval = false;
	unsigned msg = 0;

	p_object->get_data_bool(boolval);

	if (guid == standard_config_objects::bool_playlist_stop_after_current)
		msg = UWM_PLAYLIST_STOP_AFTER_CURRENT;
	else if (guid == standard_config_objects::bool_cursor_follows_playback)
		msg = UWM_CURSOR_FOLLOW_PLAYBACK;
	else
		msg = UWM_PLAYBACK_FOLLOW_CURSOR;

	g_get_panel_notifier().notify_all_async(msg, TO_VARIANT_BOOL(boolval), 0);
}

void stat_collector_notifier::on_item_played(metadb_handle_ptr p_item)
{
	notify_callback * p_callback = new notify_callback(p_item);

	g_get_panel_notifier().notify_all_callback(UWM_ON_ITEM_PLAYED, 0, (LPARAM)p_item.get_ptr(), p_callback);
}
