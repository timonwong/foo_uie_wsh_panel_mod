#include "stdafx.h"
#include "script_interface_dragsource_impl.h"
#include "script_interface_datatransfer_impl.h"


DragSourceObject::DragSourceObject()
{
    m_dragSourceHelper.CreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER);
}

STDMETHODIMP DragSourceObject::StartDrag(__interface IDataTransferObject * dataTransfer)
{
    TRACK_FUNCTION();

    //DoDragDrop();
    return S_OK;
}
