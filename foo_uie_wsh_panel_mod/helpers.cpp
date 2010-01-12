#include "stdafx.h"
#include "helpers.h"
#include "script_interface_impl.h"
#include "host.h"
#include "user_message.h"

namespace helpers
{
	// p_out must be NULL
	bool find_context_command_recur(const char * p_command, pfc::string_base & p_path, contextmenu_node * p_parent, contextmenu_node *& p_out)
	{
		if (p_parent != NULL && p_parent->get_type() == contextmenu_item_node::TYPE_POPUP)
		{
			for (t_size child_id = 0; child_id < p_parent->get_num_children(); ++child_id)
			{
				pfc::string8_fast path;
				contextmenu_node * child = p_parent->get_child(child_id);

				if (child)
				{
					const char * name = child->get_name();

					path = p_path;
					path += name;

					switch (child->get_type())
					{
					case contextmenu_item_node::TYPE_POPUP:
						path += "/";

						if (find_context_command_recur(p_command, path, child, p_out))
							return true;

						break;

					case contextmenu_item_node::TYPE_COMMAND:
						if (_stricmp(path, p_command) == 0)
						{
							p_out = child;
							return true;
						}
						else
						{
							t_size len_path = path.get_length();
							t_size len_cmd = strlen(p_command);

							if (len_path > len_cmd && path[len_path - len_cmd - 1] == '/')
							{
								if (_stricmp(path + (len_path - len_cmd), p_command) == 0)
								{
									p_out = child;
									return true;
								}
							}
						}
						break;
					}
				}
			}
		}

		return false;
	}

	bool execute_context_command_by_name(const char * p_name, metadb_handle * p_handle /*= NULL*/)
	{
		service_ptr_t<contextmenu_manager> cm;
		pfc::string8_fast dummy("");
		contextmenu_node * node = NULL;

		contextmenu_manager::g_create(cm);

		if (p_handle)
		{
			cm->init_context(pfc::list_single_ref_t<metadb_handle_ptr>(p_handle), 0);
		}
		else
		{
			cm->init_context_now_playing(0);
		}

		if (!find_context_command_recur(p_name, dummy, cm->get_root(), node))
			return false;

		if (node)
		{
			node->execute();
			return true;
		}

		return false;
	}

	static void gen_mainmenu_group_map(pfc::map_t<GUID, mainmenu_group::ptr> & p_group_guid_text_map)
	{
		service_enum_t<mainmenu_group> e;
		service_ptr_t<mainmenu_group> ptr;

		while (e.next(ptr))
		{
			GUID guid = ptr->get_guid();
			p_group_guid_text_map.find_or_add(guid) = ptr;
		}
	}

	bool execute_mainmenu_command_by_name(const char * p_name)
	{
		// First generate a map of all mainmenu_group
		pfc::map_t<GUID, mainmenu_group::ptr> group_guid_text_map;
		gen_mainmenu_group_map(group_guid_text_map);

		// Second, generate a list of all mainmenu commands
		service_enum_t<mainmenu_commands> e;
		service_ptr_t<mainmenu_commands> ptr;

		while (e.next(ptr))
		{
			for (t_uint32 idx = 0; idx < ptr->get_command_count(); ++idx)
			{
				service_ptr_t<mainmenu_group> group_ptr;
				service_ptr_t<mainmenu_group_popup> group_popup_ptr;
				GUID group_guid;
				pfc::string8_fast temp;
				pfc::string8_fast command;
				t_size len = strlen(p_name);

				ptr->get_name(idx, command);
				group_guid = ptr->get_parent();

				while (group_guid_text_map.have_item(group_guid))
				{
					group_ptr = group_guid_text_map[group_guid];

					if (group_ptr->service_query_t(group_popup_ptr))
					{
						group_popup_ptr->get_display_string(temp);
						temp.add_char('/');
						temp += command;
						command = temp;
					}

					group_guid = group_ptr->get_parent();
				}

				// command?
				if (len == command.get_length())
				{
					if (_stricmp(p_name, command) == 0)
					{
						ptr->execute(idx, NULL);
						return true;
					}
				}
				else if (len < command.get_length())
				{
					if (command[command.get_length() - len - 1] == '/')
					{
						if (_stricmp(command.get_ptr() + command.get_length() - len, p_name) == 0)
						{
							ptr->execute(idx, NULL);
							return true;
						}
					}
				}
			}
		}

		return false;
	}

	t_size calc_text_width(const Gdiplus::Font & fn, LPCTSTR text, int len)
	{
		HDC hDC;
		HFONT hFont;
		LOGFONT logfont;
		HFONT oldfont;
		Gdiplus::Bitmap bmp(5, 5, PixelFormat32bppARGB);
		Gdiplus::Graphics g(&bmp);
		SIZE sz;

		fn.GetLogFontW(&g, &logfont);
		hFont = CreateFontIndirect(&logfont);
		hDC = g.GetHDC();
		oldfont = SelectFont(hDC, hFont);
		GetTextExtentPoint32(hDC, text, len, &sz);
		SelectFont(hDC, oldfont);
		g.ReleaseHDC(hDC);

		return sz.cx;
	}

	int int_from_hex_digit(int ch)
	{
		if ((ch >= '0') && (ch <= '9'))
		{
			return ch - '0';
		}
		else if (ch >= 'A' && ch <= 'F')
		{
			return ch - 'A' + 10;
		}
		else if (ch >= 'a' && ch <= 'f')
		{
			return ch - 'a' + 10;
		}
		else
		{
			return 0;
		}
	}

	int int_from_hex_byte(const char * hex_byte)
	{
		return (int_from_hex_digit(hex_byte[0]) << 4) | (int_from_hex_digit(hex_byte[1]));
	}

	const GUID & convert_artid_to_guid(int art_id)
	{
		switch (art_id)
		{
		default:
		case 0:
			return album_art_ids::cover_front;
			break;

		case 1:
			return album_art_ids::cover_back;
			break;

		case 2:
			return album_art_ids::disc;
			break;

		case 3:
			return album_art_ids::icon;
			break;

		case 4:
			return album_art_ids::artist;
			break;
		}
	}

	bool read_album_art_into_bitmap(const album_art_data_ptr & data, Gdiplus::Bitmap ** bitmap)
	{
		*bitmap = NULL;

		if (!data.is_valid())
			return false;

		// Using IStream
		IStreamPtr is;
		Gdiplus::Bitmap * bmp = NULL;
		bool ret = true;
		HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &is);

		if (SUCCEEDED(hr) && bitmap && is)
		{
			ULONG bytes_written = 0;

			hr = is->Write(data->get_ptr(), data->get_size(), &bytes_written);

			if (SUCCEEDED(hr) && bytes_written == data->get_size())
			{
				bmp = new Gdiplus::Bitmap(is);

				if (!check_gdiplus_object(bmp))
				{
					ret = false;
					if (bmp) delete bmp;
					bmp = NULL;
				}
			}
		}

		*bitmap = bmp;
		return ret;
	}

	HRESULT get_album_art(BSTR rawpath, IGdiBitmap ** pp, int art_id, VARIANT_BOOL need_stub)
	{
		if (!rawpath) return E_INVALIDARG;
		if (!pp) return E_POINTER;

		GUID art_guid;
		album_art_data_ptr data;
		album_art_manager_instance_ptr aami = static_api_ptr_t<album_art_manager>()->instantiate();
		abort_callback_dummy abort;

		aami->open(pfc::stringcvt::string_utf8_from_wide(rawpath), abort);
		art_guid = helpers::convert_artid_to_guid(art_id);

		try
		{
			data = aami->query(art_guid, abort);
		}
		catch (std::exception &)
		{
			if (need_stub != 0)
			{
				try
				{
					data = aami->query_stub_image(abort);
				}
				catch (std::exception &)
				{
				}
			}
		}

		aami->close();

		Gdiplus::Bitmap * bitmap = NULL;
		IGdiBitmap * ret = NULL;

		if (helpers::read_album_art_into_bitmap(data, &bitmap))
			ret = new com_object_impl_t<GdiBitmap>(bitmap);

		(*pp) = ret;
		return S_OK;
	}

	HRESULT get_album_art_v2(const metadb_handle_ptr & handle, IGdiBitmap ** pp, int art_id, VARIANT_BOOL need_stub)
	{
		if (handle.is_empty()) return E_INVALIDARG;
		if (!pp) return E_POINTER;

		GUID art_guid;
		abort_callback_dummy abort;
		static_api_ptr_t<album_art_manager_v2> aamv2;
		album_art_extractor_instance_v2::ptr aaeiv2;
		IGdiBitmap * ret = NULL;

		aaeiv2 = aamv2->open(pfc::list_single_ref_t<metadb_handle_ptr>(handle), 
			pfc::list_single_ref_t<GUID>(helpers::convert_artid_to_guid(art_id)), abort);

		art_guid = helpers::convert_artid_to_guid(art_id);

		try
		{
			album_art_data_ptr data = aaeiv2->query(art_guid, abort);
			Gdiplus::Bitmap * bitmap = NULL;

			if (helpers::read_album_art_into_bitmap(data, &bitmap))
			{
				ret = new com_object_impl_t<GdiBitmap>(bitmap);
			}
		}
		catch (std::exception &)
		{
			if (need_stub)
			{
				album_art_extractor_instance_v2::ptr aaeiv2_stub = aamv2->open_stub(abort);

				try 
				{
					album_art_data_ptr data = aaeiv2_stub->query(art_guid, abort);
					Gdiplus::Bitmap * bitmap = NULL;

					if (helpers::read_album_art_into_bitmap(data, &bitmap))
					{
						ret = new com_object_impl_t<GdiBitmap>(bitmap);
					}
				} catch (std::exception &) {}
			}
		}

		(*pp) = ret;
		return S_OK;
	}

	HRESULT get_album_art_embedded(BSTR rawpath, IGdiBitmap ** pp, int art_id)
	{
		if (!rawpath) return E_INVALIDARG;
		if (!pp) return E_POINTER;

		service_enum_t<album_art_extractor> e;
		service_ptr_t<album_art_extractor> ptr;
		pfc::stringcvt::string_utf8_from_wide urawpath(rawpath);
		pfc::string_simple ext = pfc::string_extension(file_path_display(urawpath));
		abort_callback_dummy abort;
		IGdiBitmap * ret = NULL;

		while (e.next(ptr))
		{
			if (ptr->is_our_path(urawpath, ext))
			{
				album_art_extractor_instance_ptr aaep;
				GUID art_guid = helpers::convert_artid_to_guid(art_id);

				try
				{
					aaep = ptr->open(NULL, urawpath, abort);

					Gdiplus::Bitmap * bitmap = NULL;
					album_art_data_ptr data = aaep->query(art_guid, abort);

					if (helpers::read_album_art_into_bitmap(data, &bitmap))
					{
						ret = new com_object_impl_t<GdiBitmap>(bitmap);
						break;
					}
				}
				catch (std::exception &)
				{
				}
			}
		}

		(*pp) = ret;
		return S_OK;
	}

	bool read_file(const char * path, pfc::string_base & content)
	{
		HANDLE hFile = uCreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, 
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		HANDLE hFileMapping = uCreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

		if (hFileMapping == NULL)
		{
			CloseHandle(hFile);
			return false;
		}

		// 
		DWORD dwFileSize;
		dwFileSize = GetFileSize(hFile, NULL);

		LPCBYTE pAddr = (LPCBYTE)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);

		if (pAddr == NULL)
		{
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		if (dwFileSize == INVALID_FILE_SIZE)
		{
			UnmapViewOfFile(pAddr);
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		// Okay, now it's time to read
		bool status = false;

		if (dwFileSize > 3)
		{
			// UTF16 LE
			if (pAddr[0] == 0xFF && pAddr[1] == 0xFE)
			{
				const wchar_t * pSource = (const wchar_t *)(pAddr + 2);
				t_size len = (dwFileSize >> 1) - 1;

				content = pfc::stringcvt::string_utf8_from_wide(pSource, len);
				status = true;
			}
			// UTF8?
			else if (pAddr[0] == 0xEF && pAddr[1] == 0xBB && pAddr[2] == 0xBF)
			{
				const char * pSource = (const char *)(pAddr + 3);
				t_size len = dwFileSize - 3;

				content.set_string(pSource, len);
				status = true;
			}
		}

		// ANSI?
		if (!status)
		{
			content = pfc::stringcvt::string_utf8_from_ansi((const char *)pAddr, dwFileSize);
			status = true;
		}

		UnmapViewOfFile(pAddr);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return status;
	}

	bool read_file_wide(const wchar_t * path, pfc::array_t<wchar_t> & content)
	{
		HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, 
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

		if (hFileMapping == NULL)
		{
			CloseHandle(hFile);
			return false;
		}

		// 
		DWORD dwFileSize;
		dwFileSize = GetFileSize(hFile, NULL);

		LPCBYTE pAddr = (LPCBYTE)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);

		if (pAddr == NULL)
		{
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		if (dwFileSize == INVALID_FILE_SIZE)
		{
			UnmapViewOfFile(pAddr);
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		// Okay, now it's time to read
		bool status = false;

		if (dwFileSize > 3)
		{
			// UTF16 LE
			if (pAddr[0] == 0xFF && pAddr[1] == 0xFE)
			{
				const wchar_t * pSource = (const wchar_t *)(pAddr + 2);
				t_size len = (dwFileSize - 2) >> 1;

				content.set_size(len + 1);
				pfc::__unsafe__memcpy_t(content.get_ptr(), pSource, len);
				content[len] = 0;
				status = true;
			}
			// UTF8?
			else if (pAddr[0] == 0xEF && pAddr[1] == 0xBB && pAddr[2] == 0xBF)
			{
				const char * pSource = (const char *)(pAddr + 3);
				t_size pSourceSize = dwFileSize - 3;

				const t_size size = pfc::stringcvt::estimate_utf8_to_wide_quick(pSource, pSourceSize);
				content.set_size(size);
				pfc::stringcvt::convert_utf8_to_wide(content.get_ptr(), size, pSource, pSourceSize);
				status = true;
			}
		}

		// ANSI?
		if (!status)
		{
			const char * pSource = (const char *)(pAddr);
			t_size pSourceSize = dwFileSize;

			const t_size size = pfc::stringcvt::estimate_ansi_to_wide(pSource, pSourceSize);
			content.set_size(size);
			pfc::stringcvt::convert_ansi_to_wide(content.get_ptr(), size, pSource, pSourceSize);
			status = true;
		}

		UnmapViewOfFile(pAddr);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return status;
	}

	bool write_file(const char * path, const pfc::string_base & content)
	{
		HANDLE hFile = uCreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		HANDLE hFileMapping = uCreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, content.get_length() + 3, NULL);

		if (hFileMapping == NULL)
		{
			CloseHandle(hFile);
			return false;
		}

		PBYTE pAddr = (PBYTE)MapViewOfFile(hFileMapping, FILE_MAP_WRITE, 0, 0, 0);

		if (pAddr == NULL)
		{
			CloseHandle(hFileMapping);
			CloseHandle(hFile);
			return false;
		}

		const BYTE utf8_bom[] = {0xef, 0xbb, 0xbf};
		memcpy(pAddr, utf8_bom, 3);
		memcpy(pAddr + 3, content.get_ptr(), content.get_length());

		UnmapViewOfFile(pAddr);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
		return true;
	}

	file_info_pairs_filter::file_info_pairs_filter(const metadb_handle_ptr & p_handle, 
		const t_field_value_map & p_field_value_map, 
		const char * p_multivalue_field /*= NULL*/) 
		: m_handle(p_handle)
		, m_filed_value_map(p_field_value_map)
	{
		if (p_multivalue_field)
		{
			// to upper first
			string_upper multivalue_field_upper(p_multivalue_field);

			pfc::splitStringSimple_toList(m_multivalue_fields, ";", multivalue_field_upper);
		}
	}

	bool file_info_pairs_filter::apply_filter(metadb_handle_ptr p_location,t_filestats p_stats,file_info & p_info)
	{
		if (p_location == m_handle)
		{
			for (t_field_value_map::const_iterator iter = m_filed_value_map.first(); iter.is_valid(); ++iter)
			{
				if (iter->m_key.is_empty())
					continue;

				p_info.meta_remove_field(iter->m_key);

				if (!iter->m_value.is_empty())
				{
					if (m_multivalue_fields.find_item(string_upper(iter->m_key)))
					{
						// Yes, a multivalue field
						pfc::string_list_impl valuelist;
						
						// *Split value with ";"*
						pfc::splitStringSimple_toList(valuelist, ";", iter->m_value);

						for (t_size i = 0; i < valuelist.get_count(); ++i)
						{
							p_info.meta_add(iter->m_key, valuelist[i]);
						}
					}
					else
					{
						// Not a multivalue field
						p_info.meta_set(iter->m_key, iter->m_value);
					}
				}
			}

			return true;
		}

		return false;
	}

	void album_art_async::thread_proc()
	{
		FbMetadbHandle * handle = NULL;
		IGdiBitmap * bitmap = NULL;

		if (m_handle.is_valid())
		{
			if (m_only_embed)
			{
				get_album_art_embedded(m_rawpath, &bitmap, m_art_id);
			}
			else
			{
				get_album_art_v2(m_handle, &bitmap, m_art_id, m_need_stub);
			}

			handle = new com_object_impl_t<FbMetadbHandle>(m_handle);
		}

		t_param param(handle, m_art_id, bitmap);

		SendMessage(m_notify_hwnd, CALLBACK_UWM_GETALBUMARTASYNCDONE, 0, (LPARAM)&param);
	}
}
