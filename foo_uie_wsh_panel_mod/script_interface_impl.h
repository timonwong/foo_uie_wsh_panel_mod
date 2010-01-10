#pragma once

#include "script_interface.h"
#include "dbgtrace.h"

//-- IUnknown ---
#define BEGIN_COM_QI_IMPL() \
	public:\
		STDMETHOD(QueryInterface)(REFIID riid, void** pp) { \
			if (!pp) return E_INVALIDARG; \
			IUnknown * temp = NULL;

// C2594: ambiguous conversions
#define COM_QI_ENTRY_MULTI(Ibase, Iimpl) \
		if (riid == __uuidof(Ibase)) { \
			temp = static_cast<Ibase *>(static_cast<Iimpl *>(this)); \
			goto qi_entry_done; \
		}

#define COM_QI_ENTRY(Iimpl) \
			COM_QI_ENTRY_MULTI(Iimpl, Iimpl);

#define END_COM_QI_IMPL() \
			*pp = NULL; \
			return E_NOINTERFACE; \
		qi_entry_done: \
			if (temp) temp->AddRef(); \
			*pp = temp; \
			return S_OK; \
		} \
	private:

//-- IDispatch --
template<class T>
class MyIDispatchImpl: public T
{
protected:
	static ITypeInfoPtr g_typeinfo;

	MyIDispatchImpl<T>()
	{
		if (!g_typeinfo && g_typelib)
		{
			g_typelib->GetTypeInfoOfGuid(__uuidof(T), &g_typeinfo);
		}
	}

	virtual ~MyIDispatchImpl<T>()
	{
	}

	virtual void FinalRelease()
	{
	}

public:
	STDMETHOD(GetTypeInfoCount)(unsigned int * n)
	{
		if (!n) return E_INVALIDARG;

		*n = 1;
		return S_OK;
	}

	STDMETHOD(GetTypeInfo)(unsigned int i, LCID lcid, ITypeInfo** pp)
	{
		if (!g_typeinfo) return E_NOINTERFACE;
		if (!pp) return E_POINTER;
		if (i != 0) return DISP_E_BADINDEX;

		g_typeinfo->AddRef();
		(*pp) = g_typeinfo;
		return S_OK;
	}

	STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** names, unsigned int cnames, LCID lcid, DISPID* dispids)
	{
		if (!IsEqualIID(riid, IID_NULL)) return DISP_E_UNKNOWNINTERFACE;
		if (!g_typeinfo) return E_NOINTERFACE;

		return g_typeinfo->GetIDsOfNames(names, cnames, dispids);
	}

	STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD flag, DISPPARAMS* params, VARIANT* result, EXCEPINFO* excep, unsigned int* err)
	{
		if (!IsEqualIID(riid, IID_NULL)) return DISP_E_UNKNOWNINTERFACE;
		if (!g_typeinfo) return E_POINTER;

		TRACK_THIS_DISPATCH(g_typeinfo, dispid, flag);
		return g_typeinfo->Invoke(this, dispid, flag, params, result, excep, err);
	}
};

template<class T>
FOOGUIDDECL ITypeInfoPtr MyIDispatchImpl<T>::g_typeinfo;

template <typename _Base, bool _AddRef = true>
class com_object_impl_t : public _Base
{
protected:
	volatile LONG m_dwRef;

public:
	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_dwRef);
	}

	STDMETHODIMP_(ULONG) Release()
	{
		ULONG nRef = InterlockedDecrement(&m_dwRef);
		
		if (nRef == 0)
		{
			FinalRelease();
			delete this;
		} 
		
		return nRef; 
	}

private:
	inline void _construct()
	{
		m_dwRef = 0; 

		if (_AddRef)
			AddRef();
	}

public:
	virtual ~com_object_impl_t()
	{
	}

	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD_WITH_INITIALIZER(com_object_impl_t, _Base, { _construct(); })
};


//-- IDispatch impl -- [T] [IDispatch] [IUnknown]
template<class T>
class IDispatchImpl3: public MyIDispatchImpl<T>
{
	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDispatch)
		COM_QI_ENTRY(T)
		COM_QI_ENTRY(IDispatch)
	END_COM_QI_IMPL()

protected:
	IDispatchImpl3<T>() {}

	virtual ~IDispatchImpl3<T>() {}
};

//-- IDisposable impl -- [T] [IDisposable] [IDispatch] [IUnknown]
template<class T>
class IDisposableImpl4: public MyIDispatchImpl<T>
{
	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDispatch)
		COM_QI_ENTRY(T)
		COM_QI_ENTRY(IDisposable)
		COM_QI_ENTRY(IDispatch)
	END_COM_QI_IMPL()

protected:
	IDisposableImpl4<T>() {}

	virtual ~IDisposableImpl4() { }

public:
	STDMETHODIMP Dispose()
	{
		FinalRelease();
		return S_OK;
	}
};

template<class T, class T2>
class GdiObj : public MyIDispatchImpl<T>
{
	BEGIN_COM_QI_IMPL()
		COM_QI_ENTRY_MULTI(IUnknown, IDispatch)
		COM_QI_ENTRY(T)
		COM_QI_ENTRY(IGdiObj)
		COM_QI_ENTRY(IDisposable)
		COM_QI_ENTRY(IDispatch)
	END_COM_QI_IMPL()

protected:
	T2 * m_ptr;

	GdiObj<T, T2>(T2* p) : m_ptr(p)
	{
	}

	virtual ~GdiObj<T, T2>() { }

	virtual void FinalRelease()
	{
		if (m_ptr)
		{
			delete m_ptr;
			m_ptr = NULL;
		}
	}

public:
	// Default Dispose
	STDMETHODIMP Dispose()
	{
		FinalRelease();
		return S_OK;
	}

	STDMETHODIMP get__ptr(void ** pp)
	{
		*pp = m_ptr;
		return S_OK;
	}
};

class GdiFont : public GdiObj<IGdiFont, Gdiplus::Font>
{
protected:
	HFONT m_hFont;

	GdiFont(Gdiplus::Font* p, HFONT hFont): GdiObj<IGdiFont, Gdiplus::Font>(p), m_hFont(hFont) {}

	virtual ~GdiFont() {}

	virtual void FinalRelease()
	{
		if (m_hFont)
		{
			DeleteFont(m_hFont);
			m_hFont = NULL;
		}

		// call parent
		GdiObj<IGdiFont, Gdiplus::Font>::FinalRelease();
	}

public:
	STDMETHODIMP get_HFont(UINT* p);
	STDMETHODIMP get_Height(UINT* p);
};

class GdiBitmap : public GdiObj<IGdiBitmap, Gdiplus::Bitmap>
{
protected:
	GdiBitmap(Gdiplus::Bitmap* p): GdiObj<IGdiBitmap, Gdiplus::Bitmap>(p) {}

public:
	STDMETHODIMP get_Width(UINT* p);
	STDMETHODIMP get_Height(UINT* p);
	STDMETHODIMP Clone(float x, float y, float w, float h, IGdiBitmap ** pp);
	STDMETHODIMP RotateFlip(UINT mode);
	STDMETHODIMP ApplyAlpha(BYTE alpha, IGdiBitmap ** pp);
	STDMETHODIMP ApplyMask(IGdiBitmap * mask, VARIANT_BOOL * p);
	STDMETHODIMP CreateRawBitmap(IGdiRawBitmap ** pp);
	STDMETHODIMP GetGraphics(IGdiGraphics ** pp);
	STDMETHODIMP ReleaseGraphics(IGdiGraphics * p);
	STDMETHODIMP BoxBlur(int radius, int iteration);
	STDMETHODIMP Resize(UINT w, UINT h, IGdiBitmap ** pp);
};

class GdiGraphics : public GdiObj<IGdiGraphics, Gdiplus::Graphics>
{
protected:
	GdiGraphics(): GdiObj<IGdiGraphics, Gdiplus::Graphics>(NULL) {}

	void GetRoundRectPath(Gdiplus::GraphicsPath & gp, Gdiplus::RectF & rect, float arc_width, float arc_height);

public:
	STDMETHODIMP Dispose()
	{
		return E_NOTIMPL;
	}

	STDMETHODIMP put__ptr(void * p);
	STDMETHODIMP FillSolidRect(float x, float y, float w, float h, DWORD color);
	STDMETHODIMP FillGradRect(float x, float y, float w, float h, float angle, DWORD color1, DWORD color2);
	STDMETHODIMP FillRoundRect(float x, float y, float w, float h, float arc_width, float arc_height, DWORD color);
	STDMETHODIMP FillEllipse(float x, float y, float w, float h, DWORD color);
	STDMETHODIMP DrawLine(float x1, float y1, float x2, float y2, float line_width, DWORD color);
	STDMETHODIMP DrawRect(float x, float y, float w, float h, float line_width, DWORD color);
	STDMETHODIMP DrawRoundRect(float x, float y, float w, float h, float arc_width, float arc_height, float line_width, DWORD color);
	STDMETHODIMP DrawEllipse(float x, float y, float w, float h, float line_width, DWORD color);
	STDMETHODIMP DrawString(BSTR str, IGdiFont* font, DWORD color, float x, float y, float w, float h, DWORD flags);
	STDMETHODIMP GdiDrawText(BSTR str, IGdiFont * font, DWORD color, int x, int y, int w, int h, DWORD format, UINT * p);
	STDMETHODIMP DrawImage(IGdiBitmap* image, float dstX, float dstY, float dstW, float dstH, float srcX, float srcY, float srcW, float srcH, float angle, BYTE alpha);
	STDMETHODIMP GdiDrawBitmap(IGdiRawBitmap * bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH);
	STDMETHODIMP GdiAlphaBlend(IGdiRawBitmap * bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, BYTE alpha);
	STDMETHODIMP MeasureString(BSTR str, IGdiFont * font, float x, float y, float w, float h, DWORD flags, IMeasureStringInfo ** pp);
	STDMETHODIMP CalcTextWidth(BSTR str, IGdiFont * font, UINT * p);
	STDMETHODIMP SetTextRenderingHint(UINT mode);
	STDMETHODIMP SetSmoothingMode(INT mode);
	STDMETHODIMP SetInterpolationMode(INT mode);
};

class GdiRawBitmap : public IDisposableImpl4<IGdiRawBitmap>
{
protected:
	UINT m_width, m_height;
	HDC m_hdc;
	HBITMAP m_hbmp, m_hbmpold;

	GdiRawBitmap(Gdiplus::Bitmap * p_bmp);

	virtual ~GdiRawBitmap() { }

	virtual void FinalRelease()
	{
		if (m_hdc)
		{
			SelectBitmap(m_hdc, m_hbmpold);
			DeleteDC(m_hdc);
			m_hdc = NULL;
		}

		if (m_hbmp)
		{
			DeleteBitmap(m_hbmp);
			m_hbmp = NULL;
		}
	}

public:
	STDMETHODIMP get__Handle(HDC * p);
	STDMETHODIMP get_Width(UINT* p);
	STDMETHODIMP get_Height(UINT* p);
	//STDMETHODIMP GetBitmap(IGdiBitmap ** pp);
};

class MeasureStringInfo : public IDispatchImpl3<IMeasureStringInfo>
{
protected:
	float m_x, m_y, m_w, m_h;
	int m_l, m_c;

	MeasureStringInfo(float x, float y, float w, float h, int l, int c) : m_x(x), m_y(y), m_w(w), m_h(h), m_l(l), m_c(c) {}

	virtual ~MeasureStringInfo() {}

public:
	STDMETHODIMP get_x(float * p);
	STDMETHODIMP get_y(float * p);
	STDMETHODIMP get_width(float * p);
	STDMETHODIMP get_height(float * p);
	STDMETHODIMP get_lines(int * p);
	STDMETHODIMP get_chars(int * p);
};

class GdiUtils : public IDispatchImpl3<IGdiUtils>
{
protected:
	GdiUtils() {}
	virtual ~GdiUtils() {}

public:
	STDMETHODIMP Font(BSTR name, float pxSize, int style, IGdiFont** pp);
	STDMETHODIMP Image(BSTR path, IGdiBitmap** pp);
	STDMETHODIMP CreateImage(int w, int h, IGdiBitmap ** pp);
	STDMETHODIMP CreateStyleTextRender(VARIANT_BOOL pngmode, IStyleTextRender ** pp);
};

class FbFileInfo : public IDisposableImpl4<IFbFileInfo>
{
protected:
	file_info_impl * m_info_ptr;

	FbFileInfo(file_info_impl * p_info_ptr) : m_info_ptr(p_info_ptr) {}

	virtual ~FbFileInfo() { }

	virtual void FinalRelease()
	{
		if (m_info_ptr)
		{
			delete m_info_ptr;
			m_info_ptr = NULL;
		}
	}

public:
	STDMETHODIMP get__ptr(void ** pp);
	STDMETHODIMP get_MetaCount(UINT* p);
	STDMETHODIMP MetaValueCount(UINT idx, UINT* p);
	STDMETHODIMP MetaName(UINT idx, BSTR* pp);
	STDMETHODIMP MetaValue(UINT idx, UINT vidx, BSTR* pp);
	STDMETHODIMP MetaFind(BSTR name, UINT * p);
	STDMETHODIMP MetaRemoveField(BSTR name);
	STDMETHODIMP MetaAdd(BSTR name, BSTR value, UINT * p);
	STDMETHODIMP MetaInsertValue(UINT idx, UINT vidx, BSTR value);
	STDMETHODIMP get_InfoCount(UINT* p);
	STDMETHODIMP InfoName(UINT idx, BSTR* pp);
	STDMETHODIMP InfoValue(UINT idx, BSTR* pp);
	STDMETHODIMP InfoFind(BSTR name, UINT * p);
	STDMETHODIMP MetaSet(BSTR name, BSTR value);
};

class FbMetadbHandle : public IDisposableImpl4<IFbMetadbHandle>
{
protected:
	metadb_handle_ptr m_handle;

	FbMetadbHandle(const metadb_handle_ptr & src) : m_handle(src) {}

	FbMetadbHandle(metadb_handle * src) : m_handle(src) {}

	virtual ~FbMetadbHandle() { }

	virtual void FinalRelease()
	{
		m_handle.release();
	}

public:
	STDMETHODIMP get__ptr(void ** pp);
	STDMETHODIMP get_Path(BSTR* pp);
	STDMETHODIMP get_RawPath(BSTR * pp);
	STDMETHODIMP get_SubSong(UINT* p);
	STDMETHODIMP get_FileSize(LONGLONG* p);
	STDMETHODIMP get_Length(double* p);
	STDMETHODIMP GetFileInfo(IFbFileInfo ** pp);
	STDMETHODIMP UpdateFileInfo(IFbFileInfo * p);
	STDMETHODIMP UpdateFileInfoSimple(SAFEARRAY * p);
};

class FbTitleFormat : public IDisposableImpl4<IFbTitleFormat>
{
protected:
	titleformat_object::ptr m_obj;

	FbTitleFormat(BSTR expr)
	{
		pfc::stringcvt::string_utf8_from_wide utf8 = expr;
		static_api_ptr_t<titleformat_compiler>()->compile_safe(m_obj, utf8);
	}

	virtual ~FbTitleFormat() {}

	virtual void FinalRelease()
	{
		m_obj.release();
	}

public:
	STDMETHODIMP Eval(VARIANT_BOOL force, BSTR * pp);
	STDMETHODIMP EvalWithMetadb(IFbMetadbHandle * handle, BSTR * pp);
};

class ContextMenuManager : public IDisposableImpl4<IContextMenuManager>
{
protected:
	contextmenu_manager::ptr m_cm;

	ContextMenuManager() {}
	virtual ~ContextMenuManager() {}

	virtual void FinalRelease()
	{
		m_cm.release();
	}

public:
	STDMETHODIMP InitContext(IFbMetadbHandle * handle);
	STDMETHODIMP InitNowPlaying();
	STDMETHODIMP BuildMenu(IMenuObj * p, int base_id, int max_id);
	STDMETHODIMP ExecuteByID(UINT id, VARIANT_BOOL * p);
};

class MainMenuManager : public IDisposableImpl4<IMainMenuManager>
{
protected:
	mainmenu_manager::ptr m_mm;

	MainMenuManager() {}
	virtual ~MainMenuManager() {}

	virtual void FinalRelease()
	{
		m_mm.release();
	}

public:
	STDMETHODIMP Init(BSTR root_name);
	STDMETHODIMP BuildMenu(IMenuObj * p, int base_id, int count);
	STDMETHODIMP ExecuteByID(UINT id, VARIANT_BOOL * p);
};

class FbProfiler: public IDispatchImpl3<IFbProfiler>
{
protected:
	pfc::string_simple m_name;
	pfc::hires_timer m_timer;

	FbProfiler(const char * p_name) : m_name(p_name) { m_timer.start(); }
	virtual ~FbProfiler() {}

public:
	STDMETHODIMP Reset();
	STDMETHODIMP Print();
	STDMETHODIMP get_Time(double * p);
};

class FbUtils : public IDispatchImpl3<IFbUtils>
{
protected:
	FbUtils() {}
	virtual ~FbUtils() {}

public:
	STDMETHODIMP trace(SAFEARRAY * p);
	STDMETHODIMP ShowPopupMessage(BSTR msg, BSTR title, int iconid);
	STDMETHODIMP CreateProfiler(BSTR name, IFbProfiler ** pp);
	STDMETHODIMP TitleFormat(BSTR expression, IFbTitleFormat** pp);
	STDMETHODIMP GetNowPlaying(IFbMetadbHandle** pp);
	STDMETHODIMP GetFocusItem(IFbMetadbHandle** pp);
	STDMETHODIMP get_ComponentPath(BSTR* pp);
	STDMETHODIMP get_FoobarPath(BSTR* pp);
	STDMETHODIMP get_ProfilePath(BSTR* pp);
	STDMETHODIMP get_IsPlaying(VARIANT_BOOL* p);
	STDMETHODIMP get_IsPaused(VARIANT_BOOL* p);
	STDMETHODIMP get_PlaybackTime(double* p);
	STDMETHODIMP put_PlaybackTime(double time);
	STDMETHODIMP get_PlaybackLength(double* p);
	STDMETHODIMP get_PlaybackOrder(UINT* p);
	STDMETHODIMP put_PlaybackOrder(UINT order);
	STDMETHODIMP get_StopAfterCurrent(VARIANT_BOOL * p);
	STDMETHODIMP put_StopAfterCurrent(VARIANT_BOOL p);
	STDMETHODIMP get_CursorFollowPlayback(VARIANT_BOOL * p);
	STDMETHODIMP put_CursorFollowPlayback(VARIANT_BOOL p);
	STDMETHODIMP get_PlaybackFollowCursor(VARIANT_BOOL * p);
	STDMETHODIMP put_PlaybackFollowCursor(VARIANT_BOOL p);
	STDMETHODIMP get_Volume(float* p);
	STDMETHODIMP put_Volume(float value);
	STDMETHODIMP Exit();
	STDMETHODIMP Play();
	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP PlayOrPause();
	STDMETHODIMP Random();
	STDMETHODIMP Next();
	STDMETHODIMP Prev();
	STDMETHODIMP VolumeDown();
	STDMETHODIMP VolumeUp();
	STDMETHODIMP VolumeMute();
	STDMETHODIMP AddDirectory();
	STDMETHODIMP AddFiles();
	STDMETHODIMP ShowConsole();
	STDMETHODIMP ShowPreferences();
	STDMETHODIMP ClearPlaylist();
	STDMETHODIMP LoadPlaylist();
	STDMETHODIMP SavePlaylist();
	STDMETHODIMP RunMainMenuCommand(BSTR command, VARIANT_BOOL * p);
	STDMETHODIMP RunContextCommand(BSTR command, VARIANT_BOOL * p);
	STDMETHODIMP RunContextCommandWithMetadb(BSTR command, IFbMetadbHandle * handle, VARIANT_BOOL * p);
	STDMETHODIMP CreateContextMenuManager(IContextMenuManager ** pp);
	STDMETHODIMP CreateMainMenuManager(IMainMenuManager ** pp);
	STDMETHODIMP IsMetadbInMediaLibrary(IFbMetadbHandle * handle, VARIANT_BOOL * p);
};

class MenuObj : public IDisposableImpl4<IMenuObj>
{
protected:
	HMENU m_hMenu;
	HWND  m_wnd_parent;

	MenuObj(HWND wnd_parent) : m_wnd_parent(wnd_parent)
	{
		m_hMenu = ::CreatePopupMenu();
	}

	virtual ~MenuObj() { }

	virtual void FinalRelease()
	{
		if (m_hMenu && IsMenu(m_hMenu))
		{
			DestroyMenu(m_hMenu);
			m_hMenu = NULL;
		}
	}

public:
	STDMETHODIMP get_ID(UINT * p);
	STDMETHODIMP AppendMenuItem(UINT flags, UINT item_id, BSTR text);
	STDMETHODIMP AppendMenuSeparator();
	STDMETHODIMP EnableMenuItem(UINT item_id, UINT enable);
	STDMETHODIMP CheckMenuItem(UINT item_id, UINT check);
	STDMETHODIMP CheckMenuRadioItem(UINT first, UINT last, UINT check);
	STDMETHODIMP TrackPopupMenu(int x, int y, UINT * item_id);
};

class TimerObj : public IDisposableImpl4<ITimerObj>
{
protected:
	UINT m_id;

	TimerObj(UINT id) : m_id(id) {}

	virtual ~TimerObj() { }

	virtual void FinalRelease()
	{		
		if (m_id != 0)
		{
			timeKillEvent(m_id);
			m_id = 0;
		}
	}

public:
	STDMETHODIMP get_ID(UINT * p);
};

class WSHUtils : public IDispatchImpl3<IWSHUtils>
{
protected:
	WSHUtils() {}

	virtual ~WSHUtils() {}

public:
	STDMETHODIMP CheckComponent(BSTR name, VARIANT_BOOL is_dll, VARIANT_BOOL * p);
	STDMETHODIMP CheckFont(BSTR name, VARIANT_BOOL * p);
	STDMETHODIMP GetAlbumArt(BSTR rawpath, int art_id, VARIANT_BOOL need_stub, IGdiBitmap ** pp);
	STDMETHODIMP GetAlbumArtV2(IFbMetadbHandle * handle, int art_id, VARIANT_BOOL need_stub, IGdiBitmap **pp);
	STDMETHODIMP GetAlbumArtEmbedded(BSTR rawpath, int art_id, IGdiBitmap ** pp);
	STDMETHODIMP GetAlbumArtAsync(UINT window_id, IFbMetadbHandle * handle, int art_id, VARIANT_BOOL need_stub, VARIANT_BOOL only_embed, UINT * p);
	STDMETHODIMP ReadINI(BSTR filename, BSTR section, BSTR key, VARIANT defaultval, BSTR * pp);
	STDMETHODIMP WriteINI(BSTR filename, BSTR section, BSTR key, VARIANT val, VARIANT_BOOL * p);
	STDMETHODIMP IsKeyPressed(UINT vkey, VARIANT_BOOL * p);
	STDMETHODIMP PathWildcardMatch(BSTR pattern, BSTR str, VARIANT_BOOL * p);
	STDMETHODIMP ReadTextFile(BSTR filename, BSTR * pp);
	STDMETHODIMP GetSysColor(UINT index, DWORD * p);
	STDMETHODIMP GetSystemMetrics(UINT index, INT * p);
};

class FbTooltip : public IDisposableImpl4<IFbTooltip>
{
protected:
	HWND m_wndtooltip;
	HWND m_wndparent;
	BSTR m_tip_buffer;

	FbTooltip(HWND p_wndparent);

	virtual ~FbTooltip() { }

	virtual void FinalRelease()
	{	
		if (m_wndtooltip && IsWindow(m_wndtooltip))
		{
			DestroyWindow(m_wndtooltip);
			m_wndtooltip = NULL;
		}

		if (m_tip_buffer)
		{
			SysFreeString(m_tip_buffer);
			m_tip_buffer = NULL;
		}
	}

public:
	STDMETHODIMP Activate();
	STDMETHODIMP Deactivate();
	STDMETHODIMP SetMaxWidth(int width);
	STDMETHODIMP get_Text(BSTR * pp);
	STDMETHODIMP put_Text(BSTR text);
};

// forward declartion
namespace TextDesign
{
	class IOutlineText;
}

class StyleTextRender : public IDisposableImpl4<IStyleTextRender>
{
protected:
	TextDesign::IOutlineText * m_pOutLineText;
	bool m_pngmode;

	StyleTextRender(bool pngmode);
	virtual ~StyleTextRender() {}

	virtual void FinalRelease();

public:
	// Outline
	STDMETHODIMP OutLineText(DWORD text_color, DWORD outline_color, int outline_width);
	STDMETHODIMP DoubleOutLineText(DWORD text_color, DWORD outline_color1, DWORD outline_color2, int outline_width1, int outline_width2);
	STDMETHODIMP GlowText(DWORD text_color, DWORD glow_color, int glow_width);
	// Shadow
	STDMETHODIMP EnableShadow(VARIANT_BOOL enable);
	STDMETHODIMP ResetShadow();
	STDMETHODIMP Shadow(DWORD color, int thickness, int offset_x, int offset_y);
	STDMETHODIMP DiffusedShadow(DWORD color, int thickness, int offset_x, int offset_y);
	STDMETHODIMP SetShadowBackgroundColor(DWORD color, int width, int height);
	STDMETHODIMP SetShadowBackgroundImage(IGdiBitmap * img);
	// Render 
	STDMETHODIMP RenderStringPoint(IGdiGraphics * g, BSTR str, IGdiFont* font, int x, int y, DWORD flags, VARIANT_BOOL * p);
	STDMETHODIMP RenderStringRect(IGdiGraphics * g, BSTR str, IGdiFont* font, int x, int y, int w, int h, DWORD flags, VARIANT_BOOL * p);
	// PNG Mode only
	STDMETHODIMP SetPngImage(IGdiBitmap * img);

};
