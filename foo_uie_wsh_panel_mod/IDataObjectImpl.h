#pragma once

#include <OleIdl.h>

class IDataObjectImpl : public IDataObject
{
protected:
    struct DATAENTRY
    {
        FORMATETC fe;
        STGMEDIUM stgm;
    };

    pfc::list_t<FORMATETC> m_dragFormats;
    pfc::list_t<DATAENTRY> m_setDataList;

    HRESULT AddRefStgMedium(STGMEDIUM *pstgmIn, STGMEDIUM *pstgmOut, BOOL fCopyIn);
    IUnknown * GetCanonicalIUnknown(IUnknown* punk);

public:
    STDMETHODIMP GetData(FORMATETC *pformatetc, STGMEDIUM *pmedium);
    STDMETHODIMP GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
    STDMETHODIMP QueryGetData(FORMATETC *pformatetc);
    STDMETHODIMP GetCanonicalFormatEtc( FORMATETC *pformatectIn, FORMATETC *pformatetcOut);
    STDMETHODIMP SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
    STDMETHODIMP DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise);
};
