#pragma once

// TODO: Remove regex
class script_preprocessor
{
public:
	script_preprocessor(IActiveScriptParse * parser, GUID guid) : m_parser(parser)
	{
		pfc::dynamic_assert(parser != NULL);
		m_guidstr.set_string(pfc::print_guid(guid));
	}

	HRESULT preprocess(const wchar_t * script);

private:
	bool expand_path(pfc::array_t<wchar_t> & out);
	bool extract_preprocessor_block(const wchar_t * script, int & block_begin, int & block_end);

private:
	IActiveScriptParse * m_parser;
	pfc::string_simple m_guidstr;
};
