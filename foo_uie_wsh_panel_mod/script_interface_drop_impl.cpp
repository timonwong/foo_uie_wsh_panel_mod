#include "stdafx.h"
#include "script_interface_drop_impl.h"


STDMETHODIMP DataObjectIDispatchWrapper::GetData(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    if (pformatetc->cfFormat == CF_UNICODETEXT &&
        pformatetc->ptd == NULL && 
        (pformatetc->dwAspect & DVASPECT_CONTENT) != 0 &&
        pformatetc->lindex == -1 &&
        (pformatetc->tymed & TYMED_HGLOBAL) != 0)
    {
        pmedium->tymed = TYMED_HGLOBAL;
        GlobalAlloc(GMEM_FIXED, bytes);
        pmedium->hGlobal = whatever;
        pmedium->pUnkForRelease = NULL;
        return S_OK;
    }

    return DV_E_FORMATETC;
}

STDMETHODIMP DataObjectIDispatchWrapper::GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    return DV_E_FORMATETC;
}

STDMETHODIMP DataObjectIDispatchWrapper::QueryGetData(FORMATETC *pformatetc)
{
    if (pformatetc->cfFormat == CF_UNICODETEXT &&
        pformatetc->ptd == NULL && 
        pformatetc->dwAspect & DVASPECT_CONTENT != 0 &&
        pformatetc->lindex == -1 &&
        pformatetc->tymed == TYMED_HGLOBAL)
    {
        return S_OK;
    }

    return DV_E_FORMATETC;
}

STDMETHODIMP DataObjectIDispatchWrapper::GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut)
{
    pformatetcOut->cfFormat = CF_UNICODETEXT;
    pformatetcOut->ptd = NULL;
    pformatetcOut->dwAspect = DVASPECT_CONTENT;
    pformatetcOut->lindex = -1;
    pformatetcOut->tymed = TYMED_HGLOBAL;
    return S_OK;
}

STDMETHODIMP DataObjectIDispatchWrapper::SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    return E_NOTIMPL;
}

STDMETHODIMP DataObjectIDispatchWrapper::EnumFormatEtc(DWORD dwDirection,  IEnumFORMATETC **ppenumFormatEtc)
{
    if (dwDirection != DATADIR_GET) 
    {
        *ppenumFormatEtc = 0;
        return E_NOTIMPL;
    }

    SHCreateStdEnumFmtEtc(1, m_formatEtc, ppenumFormatEtc);
    return S_OK;
}

STDMETHODIMP DataObjectIDispatchWrapper::DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP DataObjectIDispatchWrapper::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP DataObjectIDispatchWrapper::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}
