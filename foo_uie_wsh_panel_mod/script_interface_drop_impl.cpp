#include "stdafx.h"
#include "script_interface_drop_impl.h"


STDMETHODIMP IDataObjectImpl::GetData(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    T* t = static_cast<T*>(this);

    if (pformatetc == NULL || pmedium == NULL)
        return E_INVALIDARG;

    if ((pformatetc->dwAspect & DVASPECT_CONTENT) == 0)
        return DV_E_DVASPECT;

    for (int i = 0; i < m_dragFormats.get_count(); ++i)
    {
        FORMATETC &f = m_dragFormats.get_item_ref(i);
        if (f.cfFormat == pformatetc->cfFormat && (f.tymed & pformatetc->tymed) != 0)
        {
            // Request data
            // break && return;
        }
    }

    for (int i = 0; i < m_setDataList.get_count(); ++i)
    {
        DATAENTRY &de = m_setDataList.get_item_ref(i);
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

    if (!(DVASPECT_CONTENT & pFormatetc->dwAspect))
        return DV_E_DVASPECT;

    for (int i = 0; iter < m_dragFormats.get_count(); ++i)
    {
        FORMATETC& f = *m_dragFormats.get_item_ref(i);
        if(f.cfFormat == pformatetc->cfFormat && (f.tymed & pformatetc->tymed) != 0)
            return S_OK;
    }

    for (int i = 0; i < m_setDataList.get_count(); ++i)
    {
        DATAENTRY& de = m_setDataList.get_item_ref(i);
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
    std::vector<DATAENTRY>::iterator hit = -1;
    
    for (int i = 0; i < m_setDataList.get_count(); ++i)
    {
        DATAENTRY& de = m_setDataList.get_item_ref(i);
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
        DATAENTRY& de = m_setDataList.get_item_ref(i);
        ReleaseStgMedium(&de.stgm);
        memset(&de.stgm, 0, sizeof(STGMEDIUM));
    }

    DATAENTRY& de = m_setDataList.get_item_ref(i);
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

    SHCreateStdEnumFmtEtc(1, m_formatEtc, ppenumFormatEtc);
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

template <class T>
IUnknown * IDataObjectImpl<T>::GetCanonicalIUnknown(IUnknown* punk)
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

template <class T>
HRESULT IDataObjectImpl<T>::AddRefStgMedium(STGMEDIUM *pstgmIn, STGMEDIUM *pstgmOut, BOOL fCopyIn)
{
    HRESULT hres = S_OK;
    STGMEDIUM stgmOut = *pstgmIn;

    if (pstgmIn->pUnkForRelease == NULL && !(pstgmIn->tymed & (TYMED_ISTREAM | TYMED_ISTORAGE)))
    {
        if (fCopyIn)
        {
            if (pstgmIn->tymed == TYMED_HGLOBAL)
            {
                stgmOut.hGlobal = GlobalClone(pstgmIn->hGlobal);
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
