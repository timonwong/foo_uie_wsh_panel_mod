#pragma once


#define _WIN32_WINNT 0x0501 // Require Windows XP
#define WINVER 0x0501
#define _WIN32_IE 0x600

// Define this to disable VLD
#define NO_VISUAL_LEAK_DETECTOR

#if defined(_DEBUG) && !defined(NO_VISUAL_LEAK_DETECTOR)
#include <vld.h>
#endif


#include <tchar.h>
#include <Windows.h>
#include <GdiPlus.h>
#include <ActivScp.h>
#include <ShellApi.h>
#include <CommCtrl.h>
#include <ComDef.h>
#include <ComDefSp.h>
#include <ObjSafe.h>
#include <StrSafe.h>
#include <azroles.h>
#include <vssym32.h>
#include <uxtheme.h>

// ATL/WTL
#define _WTL_USE_CSTRING
#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlframe.h>
#include <atldlgs.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlddx.h>
#include <atlcrack.h>
#include <atltheme.h>
#include <atlmisc.h>

// Other controls
#define PLAT_WIN 1
#define STATIC_BUILD 1
#define SCI_LEXER 1
#include <Scintilla.h>
#include <SciLexer.h>

// fb2k
#include "../../SDK/foobar2000.h"
// CUI
#include "../../columns_ui-sdk/ui_extension.h"

// Some marcos defined by windowsx.h should be removed
#ifdef _INC_WINDOWSX
#undef SubclassWindow
#endif
