#pragma once

#include "script_interface_dragsource.h"
#include "com_tools.h"


_COM_SMARTPTR_TYPEDEF(IDragSourceHelper, IID_IDragSourceHelper);


class DragSourceObject : public IDispatchImpl3<IDragSourceObject>
{
private:
    IDragSourceHelperPtr m_dragSourceHelper;

protected:
    DragSourceObject();

public:
    STDMETHODIMP StartDrag(__interface IDataTransferObject * dataTransfer);
};
