#pragma once

#include "script_interface_tooltip.h"
#include "com_tools.h"


class FbTooltip : public IDisposableImpl4<IFbTooltip>
{
protected:
    HWND m_wndtooltip;
    HWND m_wndparent;
    BSTR m_tip_buffer;

    FbTooltip(HWND p_wndparent);

    virtual ~FbTooltip() { }

    virtual void FinalRelease();

public:
    STDMETHODIMP get_Text(BSTR * pp);
    STDMETHODIMP put_Text(BSTR text);
    STDMETHODIMP Activate();
    STDMETHODIMP Deactivate();
    STDMETHODIMP SetMaxWidth(int width);
    STDMETHODIMP GetDelayTime(int type, INT * p);
    STDMETHODIMP SetDelayTime(int type, int time);
};
