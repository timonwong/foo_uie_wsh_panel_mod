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

	// {571A60E2-6E22-4292-9557-5688C6C1733C}
	static const GUID cfg_debug_mode = 
	{ 0x571a60e2, 0x6e22, 0x4292, { 0x95, 0x57, 0x56, 0x88, 0xc6, 0xc1, 0x73, 0x3c } };


	// {E0521E81-C2A4-4a3e-A5FC-A1E62B187053}
	static const GUID cfg_cui_warning_reported = 
	{ 0xe0521e81, 0xc2a4, 0x4a3e, { 0xa5, 0xfc, 0xa1, 0xe6, 0x2b, 0x18, 0x70, 0x53 } };
}


cfg_uint g_cfg_timeout(guid::cfg_timeout, 15); // Default is 15 seconds
cfg_bool g_cfg_safe_mode(guid::cfg_safe_mode, true);
cfg_bool g_cfg_debug_mode(guid::cfg_debug_mode, false);
cfg_bool g_cfg_cui_warning_reported(guid::cfg_cui_warning_reported, false);
