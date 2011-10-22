#include "stdafx.h"
#include "IDataObjectImpl.h"


STDMETHODIMP IDataObjectImpl::GetData(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    if (pformatetc == NULL || pmedium == NULL)
        return E_INVALIDARG;

    if ((pformatetc->dwAspect & DVASPECT_CONTENT) == 0)
        return DV_E_DVASPECT;

    for (unsigned i = 0; i < m_dragFormats.get_count(); ++i)
    {
        FORMATETC& f = m_dragFormats[i];
        if (f.cfFormat == pformatetc->cfFormat && (f.tymed & pformatetc->tymed) != 0)
        {
            // TODO: Request data...
            // break && return;
        }
    }

    for (unsigned i = 0; i < m_setDataList.get_count(); ++i)
    {
        DATAENTRY& de = m_setDataList[i];
        if (de.fe.cfFormat == pformatetc->cfFormat && 
            de.fe.dwAspect == pformatetc->dwAspect &&
            de.fe.lindex == pformatetc->lindex)
        {
            if (de.fe.tymed & pformatetc->tymed)
            {
                DATAENTRY* pde = &de;
                AddRefStgMedium(&pde->stgm, pmedium, FALSE);
                return S_OK;
            }
            else
            {
                return DV_E_TYMED;
            }
        }
    }

    return DV_E_FORMATETC;
}

STDMETHODIMP IDataObjectImpl::GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    return E_NOTIMPL;
}

STDMETHODIMP IDataObjectImpl::QueryGetData(FORMATETC *pformatetc)
{
    if(pformatetc == NULL)
        return E_INVALIDARG;

    if (!(DVASPECT_CONTENT & pformatetc->dwAspect))
        return DV_E_DVASPECT;

    for (unsigned i = 0; i < m_dragFormats.get_count(); ++i)
    {
        FORMATETC& f = m_dragFormats[i];
        if(f.cfFormat == pformatetc->cfFormat && (f.tymed & pformatetc->tymed) != 0)
            return S_OK;
    }

    for (unsigned i = 0; i < m_setDataList.get_count(); ++i)
    {
        DATAENTRY& de = m_setDataList[i];
        if (de.fe.cfFormat == pformatetc->cfFormat &&
            de.fe.dwAspect == pformatetc->dwAspect &&
            de.fe.lindex == pformatetc->lindex)
        {
            if ((de.fe.tymed & pformatetc->tymed))
                return S_OK;
            else
                return DV_E_TYMED;
        }
    }
    return DV_E_FORMATETC;
}

STDMETHODIMP IDataObjectImpl::GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut)
{
    return E_NOTIMPL;
}

STDMETHODIMP IDataObjectImpl::SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    int hit = -1;

    for (unsigned i = 0; i < m_setDataList.get_count(); ++i)
    {
        DATAENTRY& de = m_setDataList[i];
        if (de.fe.cfFormat == pformatetc->cfFormat &&
            de.fe.dwAspect == pformatetc->dwAspect &&
            de.fe.lindex == pformatetc->lindex)
        {
            if ((de.fe.tymed & pformatetc->tymed))
            {
                hit = i;
                break;
            }
        }
    }

    if (hit == -1)
    {
        DATAENTRY tmp = {0};
        tmp.fe = *pformatetc;
        m_setDataList.add_item(tmp);
        hit = m_setDataList.get_count() - 1;
    }
    else
    {
        DATAENTRY& de = m_setDataList[hit];
        ReleaseStgMedium(&de.stgm);
        memset(&de.stgm, 0, sizeof(STGMEDIUM));
    }

    DATAENTRY& de = m_setDataList[hit];
    HRESULT hres = S_OK;

    if (fRelease)
    {
        de.stgm = *pmedium;
        hres = S_OK;
    }
    else
    {
        hres = AddRefStgMedium(pmedium, &de.stgm, TRUE);
    }

    de.fe.tymed = de.stgm.tymed;

    if (GetCanonicalIUnknown(de.stgm.pUnkForRelease) ==
        GetCanonicalIUnknown(static_cast<IDataObject*>(this)))
    {
        de.stgm.pUnkForRelease->Release();
        de.stgm.pUnkForRelease = NULL;
    }

    return hres;
}

STDMETHODIMP IDataObjectImpl::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
    if (ppenumFormatEtc == NULL)
    {
        return E_INVALIDARG;
    }

    SHCreateStdEnumFmtEtc(m_dragFormats.get_count(), m_dragFormats.get_ptr(), ppenumFormatEtc);
    return S_OK;
}

STDMETHODIMP IDataObjectImpl::DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP IDataObjectImpl::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP IDataObjectImpl::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

IUnknown * IDataObjectImpl::GetCanonicalIUnknown(IUnknown* punk)
{
    IUnknown *punkCanonical = NULL;

    if (punk && SUCCEEDED(punk->QueryInterface(IID_IUnknown, (void**)&punkCanonical)))
    {
        punkCanonical->Release();
    }
    else
    {
        punkCanonical = punk;
    }

    return punkCanonical;
}

HRESULT IDataObjectImpl::AddRefStgMedium(STGMEDIUM *pstgmIn, STGMEDIUM *pstgmOut, BOOL fCopyIn)
{
    HRESULT hres = S_OK;
    STGMEDIUM stgmOut = *pstgmIn;

    if (pstgmIn->pUnkForRelease == NULL && !(pstgmIn->tymed & (TYMED_ISTREAM | TYMED_ISTORAGE)))
    {
        if (fCopyIn)
        {
            if (pstgmIn->tymed == TYMED_HGLOBAL)
            {
                // Global clone
                SIZE_T size = GlobalSize(pstgmIn->hGlobal);
                if (size == 0) return E_INVALIDARG;
                HGLOBAL hGlobal = GlobalAlloc(GMEM_FIXED, size);

                stgmOut.hGlobal = hGlobal;
                if (!stgmOut.hGlobal)
                    hres = E_OUTOFMEMORY;
                else
                    hres = DV_E_TYMED;
            }
        }
        else
        {
            stgmOut.pUnkForRelease = static_cast<IDataObject*>(this);
        }
    }

    if (SUCCEEDED(hres))
    {
        switch (stgmOut.tymed)
        {
        case TYMED_ISTREAM:
            stgmOut.pstm->AddRef();
            break;

        case TYMED_ISTORAGE:
            stgmOut.pstg->AddRef();
            break;
        }

        if (stgmOut.pUnkForRelease)
        {
            stgmOut.pUnkForRelease->AddRef();
        }

        *pstgmOut = stgmOut;
    }

    return hres;
}
