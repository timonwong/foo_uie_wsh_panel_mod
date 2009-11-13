#include "stdafx.h"
#include "global_cfg.h"


namespace guid
{
	// {D10190B5-4E52-491a-9BA6-36EB43E9291A}
	static const GUID panel_fcl_group = 
	{ 0xd10190b5, 0x4e52, 0x491a, { 0x9b, 0xa6, 0x36, 0xeb, 0x43, 0xe9, 0x29, 0x1a } };

	// {1F71BA08-B517-46c0-A08E-860452F23E07}
	static const GUID cfg_timeout = 
	{ 0x1f71ba08, 0xb517, 0x46c0, { 0xa0, 0x8e, 0x86, 0x4, 0x52, 0xf2, 0x3e, 0x7 } };

	// {4BD41299-6B5B-4cd1-9C0D-71F1751D302C}
	static const GUID cfg_safe_mode = 
	{ 0x4bd41299, 0x6b5b, 0x4cd1, { 0x9c, 0xd, 0x71, 0xf1, 0x75, 0x1d, 0x30, 0x2c } };
}

//columns_ui::fcl::group_impl_factory g_panel_fcl_group(guid::panel_fcl_group, "WSH Panel Mod Globals", "");


cfg_uint g_cfg_timeout(guid::cfg_timeout, 15); // Default is 15 seconds
cfg_bool g_cfg_safe_mode(guid::cfg_safe_mode, false);
