#pragma once


struct 
{
    IDispatchPtr m_disp;

    void ToString(wchar_t * outString)
    {
        int pid = getpid();
        UINT_PTR ptr = (UINT_PTR)m_disp.GetInterfacePtr();

        wprintf(L"%d,%d", pid, ptr);
    }
};

class DataObjectIDispatchWrapper : public IDataObject
{
protected:
    IDispatchPtr m_disp;
    FORMATETC m_formatEtc[1];

    DataObjectIDispatchWrapper(IDispatch * pdisp)
    {
        m_formatEtc[0].cfFormat = CF_UNICODETEXT;
        m_formatEtc[0].dwAspect = DVASPECT_CONTENT;
        m_formatEtc[0].lindex = -1;
        m_formatEtc[0].ptd = NULL;
        m_formatEtc[0].tymed = TYMED_HGLOBAL;

        m_disp = pdisp;
    }

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
