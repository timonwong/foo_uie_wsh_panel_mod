#include "stdafx.h"
#include "helpers.h"


// TODO: Change Version Number Every Time
DECLARE_COMPONENT_VERSION(
	"WSH Panel Mod",
	"1.2.1",
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
};

// Script TypeLib
ITypeLibPtr g_typelib;
static int g_load_status = E_OK;

class wsh_initquit : public initquit
{
public:
	void on_init()
	{
		// Check and show error message
		pfc::string8 err_msg;

		if (g_load_status != E_OK)
		{
			err_msg = "If you see this error message, that means this component will not function properly:\n\n";

			if (g_load_status & E_TYPELIB)
				err_msg += "Type Library: Load TypeLib Failed.\n\n";

			if (g_load_status & E_SCINTILLA)
				err_msg += "Scintilla: Load Scintilla Failed.\n\n";

			if (g_load_status & E_GDIPLUS)
				err_msg += "Gdiplus: Load Gdiplus Failed.\n\n";

			popup_message::g_show(err_msg, "WSH Panel Mod", popup_message::icon_error);
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
