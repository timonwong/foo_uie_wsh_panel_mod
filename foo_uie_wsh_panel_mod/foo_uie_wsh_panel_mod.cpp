#include "stdafx.h"
#include "simple_thread.h"
#include "popup_msg.h"


// Script TypeLib
ITypeLibPtr g_typelib;

#define VERSION_NUMBER_PREFIX "1.2.2"
// alpha, beta?
#define TEST_VERSION
#define TEST_VERSION_SUFFIX "Beta 1"

#ifdef TEST_VERSION
#  include <time.h>
#  define VERSION_NUMBER VERSION_NUMBER_PREFIX " " TEST_VERSION_SUFFIX
#else
#  define VERSION_NUMBER VERSION_NUMBER_PREFIX
#endif

namespace
{
	// TODO: Change Version Number Every Time
	DECLARE_COMPONENT_VERSION(
		"WSH Panel Mod",
		VERSION_NUMBER,
		"Windows Scripting Host Panel\n"
		"Modded by T.P. Wang\n\n"
		"Build: "  __TIME__ ", " __DATE__ "\n"
		"Columns UI API Version: " UI_EXTENSION_VERSION "\n"
	);

	// Is there anything not correctly loaded?
	enum t_load_status_error
	{
		E_OK = 0,
		E_TYPELIB = 1 << 0,
		E_SCINTILLA = 1 << 1,
		E_GDIPLUS = 1 << 2,
		E_EXPIRED = 1 << 3,
	};

	static int g_load_status = E_OK;

#ifdef TEST_VERSION
	bool is_expired(char const * time)
	{ 
		char s_month[4] = {0};
		int month, day, year;
		tm t = {0};
		const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

		sscanf_s(time, "%3s %2d %4d", s_month, _countof(s_month), &day, &year);

		const char * month_pos = strstr(month_names, s_month);
		month = (month_pos - month_names) / 3;

		t.tm_mon = month;
		t.tm_mday = day;
		t.tm_year = year - 1900;
		t.tm_isdst = FALSE;

		__time64_t start = _mktime64(&t);
		__time64_t end = _time64(NULL);

		// expire in ~14 days
		const __int64 secs = 15 * 60 * 60 * 24;
		return static_cast<__int64>(_difftime64(end, start)) > secs;
	}
#endif

	class wsh_initquit : public initquit
	{
	public:
		void on_init()
		{
			// HACK: popup_message services will not be initialized soon after start.
			popup_msg::g_set_service_initialized();
			check_error();
			popup_msg::g_process_pendings();
		}

		void check_error() 
		{
			// Check and show error message
			pfc::string8 err_msg;

#ifdef TEST_VERSION
			if (is_expired(__DATE__))
			{
				g_load_status |= E_EXPIRED;
			}
#endif

			if (g_load_status != E_OK)
			{
#ifdef TEST_VERSION
				if (!(g_load_status & E_EXPIRED))
				{
#endif
					err_msg = "If you see this error message, that means this component will not function properly:\n\n";

					if (g_load_status & E_TYPELIB)
						err_msg += "Type Library: Load TypeLib Failed.\n\n";

					if (g_load_status & E_SCINTILLA)
						err_msg += "Scintilla: Load Scintilla Failed.\n\n";

					if (g_load_status & E_GDIPLUS)
						err_msg += "Gdiplus: Load Gdiplus Failed.\n\n";
#ifdef TEST_VERSION
				}
				else
				{
					err_msg = "This version of WSH Panel Mod is expired, please get a new version now.\n\n";
				}
#endif

				popup_msg::g_show(err_msg, "WSH Panel Mod", popup_message::icon_error);
			}
		}

		void on_quit()
		{
			simple_thread_manager::instance().remove_all();
		}
	};

	static initquit_factory_t<wsh_initquit> g_initquit;
	CAppModule _Module;

	extern "C" BOOL WINAPI DllMain(HINSTANCE ins, DWORD reason, LPVOID lp)
	{
		static ULONG_PTR g_gdip_token;

		switch (reason)
		{
		case DLL_PROCESS_ATTACH:
			{
				_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

				// Load TypeLib
				TCHAR path[MAX_PATH + 4];
				DWORD len = GetModuleFileName(ins, path, MAX_PATH);

				path[len] = 0;

				if (FAILED(LoadTypeLibEx(path, REGKIND_NONE, &g_typelib)))
					g_load_status |= E_TYPELIB;

				// Load Scintilla
				if (!Scintilla_RegisterClasses(ins))
					g_load_status |= E_SCINTILLA;

				// Init GDI+
				Gdiplus::GdiplusStartupInput gdip_input;
				if (Gdiplus::GdiplusStartup(&g_gdip_token, &gdip_input, NULL) != Gdiplus::Ok)
					g_load_status |= E_GDIPLUS;

				// WTL
				_Module.Init(NULL, ins);
			}
			break;

		case DLL_PROCESS_DETACH:
			{
				// Term WTL
				_Module.Term();

				// Shutdown GDI+
				Gdiplus::GdiplusShutdown(g_gdip_token);

				// Free Scintilla resource
				Scintilla_ReleaseResources();
			}
			break;
		}

		return TRUE;
	}

}
