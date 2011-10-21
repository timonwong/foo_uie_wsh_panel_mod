#pragma once

#include "IDropTargetImpl.h"
#include "script_interface_impl.h"

class HostComm;

class HostDropTarget : public IDropTargetImpl
{
public:
    struct MessageParam
    {
        DWORD key_state;
        LONG x;
        LONG y;
        DropSourceAction * action;
    };

private:
    class process_dropped_items_to_playlist : public process_locations_notify 
    {
    public:
        process_dropped_items_to_playlist(int playlist_idx, bool to_select) 
            : m_playlist_idx(playlist_idx), m_to_select(to_select) {}

        void on_completion(const pfc::list_base_const_t<metadb_handle_ptr> & p_items);
        void on_aborted() {}

    private:
        bool m_to_select;
        int m_playlist_idx;
    };

private:
    DWORD m_effect;
    DropSourceAction *m_action;

    BEGIN_COM_QI_IMPL()
        COM_QI_ENTRY_MULTI(IUnknown, IDropTarget)
        COM_QI_ENTRY(IDropTarget)
    END_COM_QI_IMPL()

protected:
    virtual void FinalRelease() {}

public:
    HostDropTarget(HWND hWnd);
    virtual ~HostDropTarget();

public:
    // IDropTarget
    HRESULT OnDragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    HRESULT OnDragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    HRESULT OnDrop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    HRESULT OnDragLeave();
};
