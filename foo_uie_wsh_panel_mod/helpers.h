#pragma once

#include "simple_thread.h"
#include "script_interface.h"

#define TO_VARIANT_BOOL(v) ((v) ? (VARIANT_TRUE) : (VARIANT_FALSE))

namespace helpers
{
	extern bool find_context_command_recur(const char * p_command, pfc::string_base & p_path, contextmenu_node * p_parent, contextmenu_node *& p_out);
	extern bool execute_context_command_by_name(const char * p_name, metadb_handle * p_handle = NULL);
	extern bool execute_mainmenu_command_by_name(const char * p_name);
	extern bool get_mainmenu_item_checked(const GUID & guid);
	extern void set_mainmenu_item_checked(const GUID & guid, bool checked);

	inline int get_text_width(HDC hdc, LPCTSTR text, int len)
	{
		SIZE size;

		GetTextExtentPoint32(hdc, text, len, &size);
		return size.cx;
	}

	inline int get_text_height(HDC hdc, const wchar_t * text, int len)
	{
		SIZE size;

		GetTextExtentPoint32(hdc, text, len, &size);
		return size.cy;
	}

	inline int is_wrap_char(TCHAR current, TCHAR next)
	{
		if (next == 0) return true;

		switch (current)
		{
		case '\r':
		case '\n':
		case ' ':
		case '\t':
		case '-':
		case '?':
		case '!':
		case '|':
		case ')':
		case ']':
		case '}':
			//
		case '(':
		case '[':
		case '{':
		case '+':
		case '$':
		case '%':
		case '\\':
			return true;
		}

		return !(current < 0x80 && next < 0x80);
	}

	inline bool is_wrap_char_adv(TCHAR ch)
	{
		switch (ch)
		{
		case '(':
		case '[':
		case '{':
		case '+':
		case '$':
		case '%':
		case '\\':
			return true;
		}

		return false;
	}

	struct wrapped_item
	{
		BSTR text;
		int width;
	};

	extern void estimate_line_wrap(HDC hdc, const wchar_t * text, int len, int width, pfc::list_t<wrapped_item> & out);

	__declspec(noinline) static bool execute_context_command_by_name_SEH(const char * p_name, metadb_handle * p_handle = NULL)
	{
		bool ret = false;

		__try 
		{
			ret = execute_context_command_by_name(p_name, p_handle);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			ret = false;
		}

		return ret;
	}

	__declspec(noinline) static bool execute_mainmenu_command_by_name_SEH(const char * p_name)
	{
		bool ret = false;

		__try 
		{
			ret = execute_mainmenu_command_by_name(p_name);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			ret = false;
		}

		return ret;
	}

	inline COLORREF convert_argb_to_colorref(DWORD argb)
	{
		return RGB(argb >> RED_SHIFT, argb >> GREEN_SHIFT, argb >> BLUE_SHIFT);
	}

	inline DWORD convert_colorref_to_argb(DWORD color)
	{
		// COLORREF : 0x00bbggrr
		// ARGB : 0xaarrggbb
		return (GetRValue(color) << RED_SHIFT) | 
			(GetGValue(color) << GREEN_SHIFT) | 
			(GetBValue(color) << BLUE_SHIFT) | 
			0xff000000;
	}

	int int_from_hex_digit(int ch);
	int int_from_hex_byte(const char * hex_byte);

	template<class T>
	inline bool ensure_gdiplus_object(T * obj)
	{
		return ((obj) && (obj->GetLastStatus() == Gdiplus::Ok));
	}

	const GUID convert_artid_to_guid(int art_id);
	// bitmap must be NULL
	bool read_album_art_into_bitmap(const album_art_data_ptr & data, Gdiplus::Bitmap ** bitmap);
	HRESULT get_album_art(BSTR rawpath, IGdiBitmap ** pp, int art_id, VARIANT_BOOL need_stub);
	HRESULT get_album_art_v2(const metadb_handle_ptr & handle, IGdiBitmap ** pp, int art_id, VARIANT_BOOL need_stub, VARIANT_BOOL no_load = VARIANT_FALSE, pfc::string_base * image_path_ptr = NULL);
	HRESULT get_album_art_embedded(BSTR rawpath, IGdiBitmap ** pp, int art_id);

	static bool get_is_vista_or_later()
	{
		// Get OS version
		static DWORD dwMajorVersion = 0;

		if (!dwMajorVersion)
		{
			OSVERSIONINFO osvi = { 0 };

			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

			if (GetVersionEx(&osvi))
				dwMajorVersion = osvi.dwMajorVersion;
		}

		return (dwMajorVersion >= 6);
	}

	static pfc::string8_fast get_fb2k_path()
	{
		pfc::string8_fast path;

		uGetModuleFileName(NULL, path);
		path = pfc::string_directory(path);
		path.add_string("\\");

		return path;
	}

	static pfc::string8_fast get_fb2k_component_path()
	{
		pfc::string8_fast path;

		uGetModuleFileName(NULL, path);
		path = pfc::string_directory(path);
		path.add_string("\\components\\");

		return path;
	}

	static pfc::string8_fast get_profile_path()
	{
		pfc::string8_fast path;

		path = file_path_display(core_api::get_profile_path());
		path.fix_dir_separator('\\');

		return path;
	}

	// File r/w
	bool read_file(const char * path, pfc::string_base & content);
	bool read_file_wide(const wchar_t * path, pfc::array_t<wchar_t> & content);
	// Always save as UTF8 BOM
	bool write_file(const char * path, const pfc::string_base & content);


	class file_info_pairs_filter : public file_info_filter
	{
	public:
		typedef pfc::map_t<pfc::string_simple, pfc::string_simple> t_field_value_map;

	private:
		metadb_handle_ptr m_handle;
		t_field_value_map m_filed_value_map; 
		pfc::string_list_impl m_multivalue_fields;

	public:
		file_info_pairs_filter(const metadb_handle_ptr & p_handle, const t_field_value_map & p_field_value_map, const char * p_multivalue_field = NULL);

		bool apply_filter(metadb_handle_ptr p_location,t_filestats p_stats,file_info & p_info);
	};

	class album_art_async : public simple_thread
	{
	public:
		struct t_param
		{
			IFbMetadbHandle * handle;
			int art_id;
			IGdiBitmap * bitmap;
			pfc::stringcvt::string_wide_from_utf8 image_path;

			t_param(IFbMetadbHandle * p_handle, int p_art_id, IGdiBitmap * p_bitmap, const char * p_image_path) 
				: handle(p_handle), art_id(p_art_id), bitmap(p_bitmap), image_path(p_image_path)
			{
			}

			~t_param()
			{
				if (handle)
					handle->Release();

				if (bitmap)
					bitmap->Release();
			}
		};

	public:
		album_art_async(HWND notify_hwnd, metadb_handle * handle, int art_id, 
			VARIANT_BOOL need_stub, VARIANT_BOOL only_embed, VARIANT_BOOL no_load) 
			: m_notify_hwnd(notify_hwnd)
			, m_handle(handle)
			, m_art_id(art_id)
			, m_need_stub(need_stub)
			, m_only_embed(only_embed)
			, m_no_load(no_load)
		{
			if (m_handle.is_valid())
				m_rawpath = pfc::stringcvt::string_wide_from_utf8(m_handle->get_path());
		}

		virtual ~album_art_async()
		{
			close();
		}

	private:
		virtual void thread_proc();

	private:
		metadb_handle_ptr m_handle;
		_bstr_t m_rawpath;
		int m_art_id;
		VARIANT_BOOL m_need_stub;
		VARIANT_BOOL m_only_embed;
		VARIANT_BOOL m_no_load;
		HWND m_notify_hwnd;
	};

	class load_image_async : public simple_thread
	{
	public:
		struct t_param
		{
			int tid;
			IGdiBitmap * bitmap;
			_bstr_t path;

			t_param(int p_tid, IGdiBitmap * p_bitmap, BSTR p_path) 
				:  tid(p_tid), bitmap(p_bitmap), path(p_path)
			{
			}

			~t_param()
			{
				if (bitmap)
					bitmap->Release();
			}
		};

	public:
		load_image_async(HWND notify_wnd, BSTR path) 
			: m_notify_hwnd(notify_wnd), m_path(path)
		{}

		virtual ~load_image_async() { close(); }

	private:
		virtual void thread_proc();

	private:
		HWND m_notify_hwnd;
		_bstr_t m_path;
	};
}
