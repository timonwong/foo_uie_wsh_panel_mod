#pragma once

class popup_msg
{
public:
	static inline void g_show(const char * p_msg, const char * p_title, popup_message::t_icon p_icon = popup_message::icon_information)
	{
		if (sm_popup_service_initialized)
		{
			::popup_message::g_show(p_msg, p_title, p_icon);
		}
		else
		{
			if (!sm_popup_pendings) 
				sm_popup_pendings = new t_popup_pendings();

			if (sm_popup_pendings)
				sm_popup_pendings->add_item(t_popup_pending(p_msg, p_title, p_icon));
		}
	}

	static inline void g_process_pendings()
	{
		if (!sm_popup_pendings) return;

		for (t_popup_pendings::iterator iter = sm_popup_pendings->first(); iter.is_valid(); ++iter)
		{
			::popup_message::g_show(iter->msg, iter->title, iter->icon);
		}

		sm_popup_pendings->remove_all();
		delete sm_popup_pendings;
		sm_popup_pendings = NULL;
	}

	static inline void g_set_service_initialized()
	{
		sm_popup_service_initialized = true;
	}

public:
	struct t_popup_pending
	{
		t_popup_pending(const char * p_msg, const char * p_title, popup_message::t_icon p_icon)
			: msg(p_msg), title(p_title), icon(p_icon) {}
		t_popup_pending() {}

		pfc::string_simple msg, title;
		popup_message::t_icon icon;
	};

	typedef pfc::chain_list_v2_t<t_popup_pending> t_popup_pendings;

private:
	static bool sm_popup_service_initialized;
	static t_popup_pendings * sm_popup_pendings;
};
