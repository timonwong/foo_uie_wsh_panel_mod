#pragma once


struct t_directive_value
{
	typedef pfc::array_t<wchar_t> t_array;

	t_array directive;
	t_array value;

	t_directive_value()
	{
	}

	t_directive_value(const t_array & p_directive, const t_array & p_value) :
		directive(p_directive), 
		value(p_value)
	{
	}
};

class script_preprocessor
{
public:
	script_preprocessor(const wchar_t * script, GUID guid)
	{
		m_guidstr.set_string(pfc::print_guid(guid));
		m_is_ok = preprocess(script);
	}

	HRESULT process_import(IActiveScriptParse * parser);

private:
	bool preprocess(const wchar_t * script);
	bool scan_directive_and_value(const wchar_t *& p, const wchar_t * pend);
	bool scan_value(const wchar_t *& p, const wchar_t * pend);
	bool expand_path(pfc::array_t<wchar_t> & out);
	bool extract_preprocessor_block(const wchar_t * script, int & block_begin, int & block_end);

private:
	bool m_is_ok;
	pfc::string_simple m_guidstr;
	pfc::array_t<wchar_t> m_directive_buffer;
	pfc::array_t<wchar_t> m_value_buffer;
	pfc::list_t<t_directive_value> m_directive_value_list;
};
