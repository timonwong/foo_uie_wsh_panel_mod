#pragma once


enum t_version_info
{
	VERSION_0x73 = 0x73,
	VERSION_0x74,
	VERSION_0x75,
	VERSION_0x76,
	VERSION_0x77,
	VERSION_0x78,
	VERSION_0x79,
	CONFIG_VERSION_CURRENT = VERSION_0x79,
};

enum t_edge_style : char
{
	NO_EDGE = 0,
	SUNKEN_EDGE,
	GREY_EDGE,
};

inline DWORD edge_style_from_config(t_edge_style edge_style)
{
	switch (edge_style)
	{
	case SUNKEN_EDGE:
		return WS_EX_CLIENTEDGE;

	case GREY_EDGE:
		return WS_EX_STATICEDGE;

	default:
		return 0;
	}
}

class sci_prop_config
{
public:
	typedef pfc::string_simple t_key;
	typedef _variant_t t_val;
	typedef pfc::map_t<t_key, t_val, pfc::comparator_stricmp_ascii> t_map;

public:
	static bool g_is_allowed_type(VARTYPE p_vt);

	inline t_map & get_val() { return m_map; }
	// p_out should be inited or cleared.
	bool get_config_item(const char * p_key, VARIANT & p_out);
	void set_config_item(const char * p_key, const VARIANT & p_val);

	void load(stream_reader * reader, abort_callback & abort) throw();
	void save(stream_writer * writer, abort_callback & abort) const throw();

	static void g_load(t_map & data, stream_reader * reader, abort_callback & abort) throw();
	static void g_save(const t_map & data, stream_writer * writer, abort_callback & abort) throw();

	//static void g_import(t_map & data, stream_reader * reader, abort_callback & abort) throw();
	//static void g_export(const t_map & data, stream_writer * writer, abort_callback & abort) throw();

private:
	t_map m_map;
	static const GUID m_guid;
};

// {C389FC3A-F4DD-4a93-BC19-47D198338902}
FOOGUIDDECL const GUID sci_prop_config::m_guid = { 0xc389fc3a, 0xf4dd, 0x4a93, { 0xbc, 0x19, 0x47, 0xd1, 0x98, 0x33, 0x89, 0x2 } };

class wsh_panel_vars
{
private:
	bool m_disabled;
	bool m_grab_focus;
	bool m_pseudo_transparent;
	WINDOWPLACEMENT m_wndpl;
	pfc::string8  m_script_name;
	pfc::string8  m_script_code;
	sci_prop_config m_config_prop;
	t_edge_style m_edge_style;
	GUID m_config_guid;

public:
	wsh_panel_vars()
	{
		reset_config();
	}

	static void get_default_script_code(pfc::string_base & out);
	void reset_config();
	void load_config(stream_reader * reader, t_size size, abort_callback & abort);
	void save_config(stream_writer * writer, abort_callback & abort) const;

	inline pfc::string_base& get_script_name()
	{
		return m_script_name;
	}

	inline pfc::string_base & get_script_code()
	{
		return m_script_code;
	}

	inline bool & get_pseudo_transparent()
	{
		return m_pseudo_transparent;
	}

	inline const bool & get_pseudo_transparent() const
	{
		return m_pseudo_transparent;
	}

	inline bool & get_grab_focus()
	{
		return m_grab_focus;
	}

	inline WINDOWPLACEMENT & get_wndpl()
	{
		return m_wndpl;
	}

	inline bool & get_disabled()
	{
		return m_disabled;
	}

	inline sci_prop_config & get_config_prop()
	{
		return m_config_prop;
	}

	inline t_edge_style & get_edge_style()
	{
		return m_edge_style;
	}

	inline const t_edge_style & get_edge_style() const
	{
		return m_edge_style;
	}

	inline GUID & get_config_guid()
	{
		return m_config_guid;
	}
};
