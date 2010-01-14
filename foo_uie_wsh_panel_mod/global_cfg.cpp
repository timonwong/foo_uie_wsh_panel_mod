#include "stdafx.h"
#include "global_cfg.h"


namespace guid
{
	// {1F71BA08-B517-46c0-A08E-860452F23E07}
	static const GUID cfg_timeout = 
	{ 0x1f71ba08, 0xb517, 0x46c0, { 0xa0, 0x8e, 0x86, 0x4, 0x52, 0xf2, 0x3e, 0x7 } };

	// {8826D886-6E34-4796-9B61-1FEA996730F0}
	static const GUID cfg_safe_mode = 
	{ 0x8826d886, 0x6e34, 0x4796, { 0x9b, 0x61, 0x1f, 0xea, 0x99, 0x67, 0x30, 0xf0 } };

	// {5C356F3A-AF0E-4e1d-882C-EA6741E089A1}
	static const GUID cfg_from_dui_first_time = 
	{ 0x5c356f3a, 0xaf0e, 0x4e1d, { 0x88, 0x2c, 0xea, 0x67, 0x41, 0xe0, 0x89, 0xa1 } };
}


cfg_uint g_cfg_timeout(guid::cfg_timeout, 15); // Default is 15 seconds
cfg_bool g_cfg_safe_mode(guid::cfg_safe_mode, true);
cfg_bool g_cfg_from_dui_first_time(guid::cfg_from_dui_first_time, true);
