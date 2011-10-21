#include "stdafx.h"
#include "host.h"
#include "host_droptarget.h"


HostDropTarget::HostDropTarget(HWND hWnd) 
    : IDropTargetImpl(hWnd)
    , m_effect(DROPEFFECT_NONE)
    , m_action(new com_object_impl_t<DropSourceAction, true>())
{

}

HostDropTarget::~HostDropTarget()
{
    m_action->Release();
}

void HostDropTarget::process_dropped_items_to_playlist::on_completion(const pfc::list_base_const_t<metadb_handle_ptr> & p_items)
{
    bit_array_true selection_them;
    bit_array_false selection_none;
    bit_array * select_ptr = &selection_them;
    static_api_ptr_t<playlist_manager> pm;
    t_size playlist;

    if (m_playlist_idx == -1)
        playlist = pm->get_active_playlist();
    else
        playlist = m_playlist_idx;

    if (!m_to_select)
        select_ptr = &selection_none;

    if (playlist != pfc_infinite && playlist < pm->get_playlist_count())
    {
        pm->playlist_add_items(playlist, p_items, *select_ptr);
    }
}

HRESULT HostDropTarget::OnDragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if (!pdwEffect) return E_POINTER;

    m_action->Reset();

    ScreenToClient(m_hWnd, reinterpret_cast<LPPOINT>(&pt));
    MessageParam param = {grfKeyState, pt.x, pt.y, m_action };
    SendMessage(m_hWnd, UWM_DRAG_ENTER, 0, (LPARAM)&param);

    // Parsable?
    m_action->Parsable() = m_action->Parsable() || static_api_ptr_t<playlist_incoming_item_filter>()->process_dropped_files_check_ex(pDataObj, &m_effect);

    if (!m_action->Parsable())
        *pdwEffect = DROPEFFECT_NONE;
    else
        *pdwEffect = m_effect;
    return S_OK;
}

HRESULT HostDropTarget::OnDragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if (!pdwEffect) return E_POINTER;

    ScreenToClient(m_hWnd, reinterpret_cast<LPPOINT>(&pt));
    MessageParam param = {grfKeyState, pt.x, pt.y, m_action };
    SendMessage(m_hWnd, UWM_DRAG_OVER, 0, (LPARAM)&param);

    if (!m_action->Parsable())
        *pdwEffect = DROPEFFECT_NONE;
    else
        *pdwEffect = m_effect;

    return S_OK;
}

HRESULT HostDropTarget::OnDragLeave()
{
    SendMessage(m_hWnd, UWM_DRAG_LEAVE, 0, 0);
    return S_OK;
}

HRESULT HostDropTarget::OnDrop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if (!pdwEffect) return E_POINTER;

    ScreenToClient(m_hWnd, reinterpret_cast<LPPOINT>(&pt));
    MessageParam param = {grfKeyState, pt.x, pt.y, m_action };
    SendMessage(m_hWnd, UWM_DRAG_DROP, 0, (LPARAM)&param);

    int playlist = m_action->Playlist();
    bool to_select = m_action->ToSelect();

    if (m_action->Parsable())
    {
        switch (m_action->Mode())
        {
        case DropSourceAction::kActionModePlaylist:
            static_api_ptr_t<playlist_incoming_item_filter_v2>()->process_dropped_files_async(pDataObj, 
                playlist_incoming_item_filter_v2::op_flag_delay_ui,
                core_api::get_main_window(), new service_impl_t<process_dropped_items_to_playlist>(playlist, to_select));
            break;

        case DropSourceAction::kActionModeFilenames:
            break;

        default:
            break;
        }
    }

    if (!m_action->Parsable())
        *pdwEffect = DROPEFFECT_NONE;
    else
        *pdwEffect = m_effect;

    return S_OK;
}
