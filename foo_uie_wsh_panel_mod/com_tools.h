#pragma once

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

		TRACK_THIS_DISPATCH_CALL(g_typeinfo, dispid, flag);
		return g_typeinfo->Invoke(this, dispid, flag, params, result, excep, err);
	}
};

template<class T>
FOOGUIDDECL ITypeInfoPtr MyIDispatchImpl<T>::g_typeinfo;


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

template <typename _Base, bool _AddRef = true>
class com_object_impl_t : public _Base
{
private:
	volatile LONG m_dwRef;

	inline ULONG AddRef_()
	{
		return InterlockedIncrement(&m_dwRef);
	}

	inline ULONG Release_()
	{
		ULONG nRef = InterlockedDecrement(&m_dwRef);
		return nRef; 
	}

	inline void Construct_()
	{
		m_dwRef = 0; 
		if (_AddRef)
			AddRef_();
	}

	virtual ~com_object_impl_t()
	{
	}

public:
	STDMETHODIMP_(ULONG) AddRef()
	{
		return AddRef_();
	}

	STDMETHODIMP_(ULONG) Release()
	{
		ULONG n = Release_();
		if (n == 0)
			delete this;
		return n;
	}

	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD_WITH_INITIALIZER(com_object_impl_t, _Base, { Construct_(); })
};

template <class T>
class com_object_singleton_t
{
public:
	static inline T * instance()
	{
		if (!_instance)
		{
			insync(_cs);

			if (!_instance)
			{
				_instance = new com_object_impl_t<T, false>();
			}
		}

		return reinterpret_cast<T *>(_instance.GetInterfacePtr());
	}

private:
	static IDispatchPtr _instance;
	static critical_section _cs;

	PFC_CLASS_NOT_COPYABLE_EX(com_object_singleton_t)
};

template <class T>
FOOGUIDDECL IDispatchPtr com_object_singleton_t<T>::_instance;

template <class T>
FOOGUIDDECL critical_section com_object_singleton_t<T>::_cs;
