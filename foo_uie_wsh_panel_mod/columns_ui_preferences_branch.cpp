#include "stdafx.h"
#include "columns_ui_preferences_branch.h"

FOOGUIDDECL const GUID g_guid_columns_ui_extensions_branch =
	{ 0x74fd83fd, 0x6927, 0x43c6, { 0xb6, 0x73, 0x1, 0x27, 0x2c, 0xd8, 0xa5, 0x4e } };


namespace
{
	preferences_branch_factory g_branch(g_guid_columns_ui_extensions_branch, preferences_page_v2::guid_display, "Columns UI extensions", 0);
}
