#include "stdafx.h"
#include "script_preprocessor.h"
#include "helpers.h"
#include "deelx.h"


HRESULT script_preprocessor::preprocess(const wchar_t * script)
{
	HRESULT hr = S_OK;	
	int block_begin = 0;
	int block_end = 0;

	if (!extract_preprocessor_block(script, block_begin, block_end))
	{
		return hr;
	}

	const wchar_t regex_str[] = L"^\\s*'?\\s*//\\s*@import\\s*\"([^\"]+)\"";
	CRegexpT<wchar_t> regex(regex_str, MULTILINE);
	CContext * pcontext = regex.PrepareMatch(script, block_end, block_begin);
	MatchResult result = regex.Match(pcontext);
	
	while (result.IsMatched())
	{
		int start, end;
		pfc::array_t<wchar_t> path_buffer;

		// path
		start = result.GetGroupStart(1);
		end = result.GetGroupEnd(1);

		if (start >= 0)
		{
			path_buffer.set_size(end - start + 1);
			pfc::__unsafe__memcpy_t(path_buffer.get_ptr(), script + start, end - start);
			path_buffer[end - start] = 0;
		}
		else
		{
			path_buffer.force_reset();
		}

		// Try parse
		{
			expand_path(path_buffer);

			pfc::array_t<wchar_t> code;
			bool is_file_read = helpers::read_file_wide(path_buffer.get_ptr(), code);
			pfc::string_formatter msg;

			msg << "WSH Panel Mod (GUID: " << m_guidstr << "): "
				<< "Parsing file \"" << pfc::stringcvt::string_utf8_from_wide(path_buffer.get_ptr())
				<< "\"";

			if (!is_file_read)
			{
				msg << ": Failed to load";
			}
			
			console::formatter() << msg;

			if (is_file_read)
			{
				hr = m_parser->ParseScriptText(code.get_ptr(), NULL, NULL, NULL, NULL, 0,
					SCRIPTTEXT_ISPERSISTENT | SCRIPTTEXT_ISVISIBLE, NULL, NULL);

				if (FAILED(hr))
					break;
			}
		}

		// Next
		result = regex.Match(pcontext);
	}

	return hr;
}

bool script_preprocessor::expand_path(pfc::array_t<wchar_t> & out)
{
	typedef pfc::string8_fast (*t_func)();

	struct {
		const wchar_t * which;
		t_func func;
	} expand_table[] = {
		{ L"%fb2k_path%", helpers::get_fb2k_path },
		{ L"%fb2k_component_path%", helpers::get_fb2k_component_path },
		{ L"%fb2k_profile_path%", helpers::get_profile_path },
	};

	pfc::array_t<wchar_t> buffer;
	buffer.set_size(MAX_PATH + 1);
	//buffer.fill_null(); // Just in case

	wchar_t * p = out.get_ptr();
	wchar_t * pend = p + wcslen(p);

	wchar_t * pbuffer = buffer.get_ptr();
	wchar_t * pbuffer_max = pbuffer + MAX_PATH;

	while ((p < pend) && (pbuffer < pbuffer_max))
	{
		if (*p != '%')
		{
			*pbuffer = *p;
		}
		else
		{
			for (size_t i = 0; i < _countof(expand_table); ++i)
			{
				size_t expand_which_size = wcslen(expand_table[i].which);

				if (_wcsnicmp(p, expand_table[i].which, expand_which_size) == 0)
				{
					pfc::stringcvt::string_wide_from_utf8 expanded(expand_table[i].func());
					size_t expanded_size = expanded.length();

					if (pbuffer + expanded_size > pbuffer_max)
						return false;

					pfc::__unsafe__memcpy_t(pbuffer, expanded.get_ptr(), expanded_size);
					pbuffer += expanded_size;
					p += expand_which_size;
					continue;
				}
			}

			// No luck
			*pbuffer = *p;
		}

		++pbuffer;
		++p;
	}

	// trailing 'zero'
	*pbuffer = 0;
	// Copy
	out.set_data_fromptr(buffer.get_ptr(), pbuffer - buffer.get_ptr() + 1);
	return true;
}

bool script_preprocessor::extract_preprocessor_block(const wchar_t * script, int & block_begin, int & block_end)
{
	if (!script) return false;

	const wchar_t preprocessor_begin[] = L"// ==PREPROCESSOR==";
	const wchar_t preprocessor_end[] = L"// ==/PREPROCESSOR==";

	const wchar_t * pblock_begin = wcsstr(script, preprocessor_begin);

	if (!pblock_begin) return false;

	pblock_begin += _countof(preprocessor_begin) - 1;

	while (*pblock_begin && (*pblock_begin != '\n'))
		++pblock_begin;

	const wchar_t * pblock_end = wcsstr(pblock_begin, preprocessor_end);

	if (!pblock_end) return false;

	while ((pblock_end > pblock_begin + 1) && (*pblock_end != '\n'))
		--pblock_end;

	if (*pblock_end == '\r')
		--pblock_end;

	block_begin = pblock_begin - script;
	block_end = pblock_end - script;
	return true;
}
