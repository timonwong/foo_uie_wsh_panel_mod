#include "stdafx.h"
#include "script_interface_impl.h"
#include "script_interface_playlist_impl.h"
#include "helpers.h"
#include "com_array.h"


STDMETHODIMP FbPlaylistMangerService::GetPlaylistItems(UINT playlistIndex, IFbMetadbHandleList ** outItems)
{
    TRACK_FUNCTION();

    if (!outItems) return E_POINTER;

    metadb_handle_list items;
    static_api_ptr_t<playlist_manager>()->playlist_get_all_items(playlistIndex, items);
    (*outItems) = new com_object_impl_t<FbMetadbHandleList>(items);

    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::SetPlaylistSelectionSingle(UINT playlistIndex, UINT itemIndex, VARIANT_BOOL state)
{
    TRACK_FUNCTION();

    static_api_ptr_t<playlist_manager>()->playlist_set_selection_single(playlistIndex, itemIndex, state == VARIANT_TRUE);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::SetPlaylistSelection(UINT playlistIndex, VARIANT affectedItems, VARIANT_BOOL state)
{
    TRACK_FUNCTION();

    helpers::com_array_reader arrayReader;
    int count;

    // cannot convert, just fail
    if (!arrayReader.convert(&affectedItems)) return E_INVALIDARG;
    // no items
    if (arrayReader.get_count() == 0) return S_OK;

    static_api_ptr_t<playlist_manager> plm;
    count = plm->playlist_get_item_count(playlistIndex);

    bit_array_bittable affected(count);

    for (int i = arrayReader.get_lbound(); i < arrayReader.get_count(); ++i)
    {
        _variant_t index;

        arrayReader.get_item(i, index);
        if (FAILED(VariantChangeType(&index, &index, 0, VT_I4))) return E_INVALIDARG;

        affected.set(index.lVal, true);
    }

    bit_array_val status(state == VARIANT_TRUE);
    static_api_ptr_t<playlist_manager>()->playlist_set_selection(playlistIndex, affected, status);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::ClearPlaylistSelection(UINT playlistIndex)
{
    TRACK_FUNCTION();

    static_api_ptr_t<playlist_manager>()->playlist_clear_selection(playlistIndex);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::GetPlaylistFocusItemHandle(VARIANT_BOOL force, IFbMetadbHandle ** outItem)
{
    TRACK_FUNCTION();

    if (!outItem) return E_POINTER;

    metadb_handle_ptr metadb;

    try
    {
        // Get focus item
        static_api_ptr_t<playlist_manager>()->activeplaylist_get_focus_item_handle(metadb);

        if (force && metadb.is_empty())
        {
            // if there's no focused item, just try to get the first item in the *active* playlist
            static_api_ptr_t<playlist_manager>()->activeplaylist_get_item_handle(metadb, 0);
        }

        if (metadb.is_empty())
        {
            (*outItem) = NULL;
            return S_OK;
        }
    }
    catch (std::exception &) {}

    (*outItem) = new com_object_impl_t<FbMetadbHandle>(metadb);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::SetPlaylistFocusItem(UINT playlistIndex, UINT itemIndex)
{
    TRACK_FUNCTION();

    static_api_ptr_t<playlist_manager>()->playlist_set_focus_item(playlistIndex, itemIndex);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::SetPlaylistFocusItemByHandle(UINT playlistIndex, IFbMetadbHandle * item)
{
    TRACK_FUNCTION();

    if (!item) return E_INVALIDARG;

    metadb_handle * ptr = NULL;
    item->get__ptr((void**)&ptr);

    static_api_ptr_t<playlist_manager>()->playlist_set_focus_by_handle(playlistIndex, ptr);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::GetPlaylistName(UINT playlistIndex, BSTR * outName)
{
    TRACK_FUNCTION();

    if (!outName) return E_POINTER;

    pfc::string8_fast temp;

    static_api_ptr_t<playlist_manager>()->playlist_get_name(playlistIndex, temp);
    *outName = SysAllocString(pfc::stringcvt::string_wide_from_utf8(temp));
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::CreatePlaylist(UINT playlistIndex, BSTR name, UINT * outPlaylistIndex)
{
    TRACK_FUNCTION();

    if (!name) return E_INVALIDARG;
    if (!outPlaylistIndex) return E_POINTER;

    if (*name)
    {
        pfc::stringcvt::string_utf8_from_wide uname(name);

        *outPlaylistIndex = static_api_ptr_t<playlist_manager>()->create_playlist(uname, uname.length(), playlistIndex);
    }
    else
    {
        *outPlaylistIndex = static_api_ptr_t<playlist_manager>()->create_playlist_autoname(playlistIndex);
    }

    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::RemovePlaylist(UINT playlistIndex, VARIANT_BOOL * outSuccess)
{
    TRACK_FUNCTION();

    if (!outSuccess) return E_POINTER;

    *outSuccess = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->remove_playlist(playlistIndex));
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::MovePlaylist(UINT from, UINT to, VARIANT_BOOL * outSuccess)
{
    TRACK_FUNCTION();

    if (!outSuccess) return E_POINTER;

    static_api_ptr_t<playlist_manager> pm;
    order_helper order(pm->get_playlist_count());

    if ((from >= order.get_count()) || (to >= order.get_count()))
    {
        *outSuccess = VARIANT_FALSE;
        return S_OK;
    }

    int inc = (from < to) ? 1 : -1;

    for (t_size i = from; i != to; i += inc)
    {
        order[i] = order[i + inc];
    }

    order[to] = from;

    *outSuccess = TO_VARIANT_BOOL(pm->reorder(order.get_ptr(), order.get_count()));
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::RenamePlaylist(UINT playlistIndex, BSTR name, VARIANT_BOOL * outSuccess)
{
    TRACK_FUNCTION();

    if (!name) return E_INVALIDARG;
    if (!outSuccess) return E_POINTER;

    pfc::stringcvt::string_utf8_from_wide uname(name);

    *outSuccess = TO_VARIANT_BOOL(static_api_ptr_t<playlist_manager>()->playlist_rename(playlistIndex, uname, uname.length()));
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::DuplicatePlaylist(UINT from, BSTR name, UINT * outPlaylistIndex)
{
    TRACK_FUNCTION();

    if (!outPlaylistIndex) return E_POINTER;

    static_api_ptr_t<playlist_manager_v4> manager;
    metadb_handle_list contents;
    pfc::string8_fast name_utf8;

    if (from >= manager->get_playlist_count()) return E_INVALIDARG;

    manager->playlist_get_all_items(from, contents);

    if (!name || !*name)
        // If no name specified, create a playlist which will have the same name
        manager->playlist_get_name(from, name_utf8);
    else
        name_utf8 = pfc::stringcvt::string_utf8_from_wide(name);

    stream_reader_dummy dummy_reader;
    abort_callback_dummy dummy_callback;

    t_size idx = manager->create_playlist_ex(name_utf8.get_ptr(), name_utf8.get_length(), from + 1, contents, &dummy_reader, dummy_callback);
    *outPlaylistIndex = idx;
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::get_PlaybackOrder(UINT * outOrder)
{
    TRACK_FUNCTION();

    if (!outOrder) return E_POINTER;

    (*outOrder) = static_api_ptr_t<playlist_manager>()->playback_order_get_active();
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::put_PlaybackOrder(UINT order)
{
    TRACK_FUNCTION();

    static_api_ptr_t<playlist_manager>()->playback_order_set_active(order);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::get_ActivePlaylist(UINT * outPlaylistIndex)
{
    TRACK_FUNCTION();

    if (!outPlaylistIndex) return E_POINTER;

    *outPlaylistIndex = static_api_ptr_t<playlist_manager>()->get_active_playlist();
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::put_ActivePlaylist(UINT playlistIndex)
{
    TRACK_FUNCTION();

    static_api_ptr_t<playlist_manager> pm;
    t_size index = (playlistIndex < pm->get_playlist_count()) ? playlistIndex : pfc::infinite_size;

    pm->set_active_playlist(index);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::get_PlayingPlaylist(UINT * outPlaylistIndex)
{
    TRACK_FUNCTION();

    if (!outPlaylistIndex) return E_POINTER;

    (*outPlaylistIndex) = static_api_ptr_t<playlist_manager>()->get_playing_playlist();
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::put_PlayingPlaylist(UINT playlistIndex)
{
    TRACK_FUNCTION();

    static_api_ptr_t<playlist_manager> pm;
    t_size index = (playlistIndex < pm->get_playlist_count()) ? playlistIndex : pfc::infinite_size;

    pm->set_playing_playlist(index);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::get_PlaylistCount(UINT * outCount)
{
    TRACK_FUNCTION();

    if (!outCount) return E_POINTER;

    *outCount = static_api_ptr_t<playlist_manager>()->get_playlist_count();
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::get_PlaylistItemCount(UINT playlistIndex, UINT * outCount)
{
    TRACK_FUNCTION();

    if (!outCount) return E_POINTER;

    *outCount = static_api_ptr_t<playlist_manager>()->playlist_get_item_count(playlistIndex);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::CreatePlaybackQueueItem(IFbPlaybackQueueItem ** outPlaybackQueueItem)
{
    TRACK_FUNCTION();

    if (!outPlaybackQueueItem) return E_POINTER;

    (*outPlaybackQueueItem) = new com_object_impl_t<FbPlaybackQueueItem>();
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::RemoveItemFromPlaybackQueue(UINT index)
{
    TRACK_FUNCTION();

    static_api_ptr_t<playlist_manager>()->queue_remove_mask(bit_array_one(index));
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::RemoveItemsFromPlaybackQueue(VARIANT affectedItems)
{
    TRACK_FUNCTION();

    helpers::com_array_reader arrayReader;
    static_api_ptr_t<playlist_manager> plman;
    int count;
   
    // cannot convert, just fail
    if (!arrayReader.convert(&affectedItems)) return E_INVALIDARG;
    // no items
    if (arrayReader.get_count() == 0) return S_OK;

    count = plman->queue_get_count();
    bit_array_bittable affected(count);

    for (int i = arrayReader.get_lbound(); i < arrayReader.get_count(); ++i)
    {
        _variant_t index;

        arrayReader.get_item(i, index);
        if (FAILED(VariantChangeType(&index, &index, 0, VT_I4))) return E_INVALIDARG;

        affected.set(index.lVal, true);
    }

    plman->queue_remove_mask(affected);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::AddPlaylistItemToPlaybackQueue(UINT playlistIndex, UINT playlistItemIndex)
{
    TRACK_FUNCTION();

    static_api_ptr_t<playlist_manager>()->queue_add_item_playlist(playlistIndex, playlistItemIndex);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::AddItemToPlaybackQueue(IFbMetadbHandle * handle)
{
    TRACK_FUNCTION();

    metadb_handle * ptrHandle = NULL;
    handle->get__ptr((void **)&ptrHandle);
    if (!ptrHandle) return E_INVALIDARG;

    static_api_ptr_t<playlist_manager>()->queue_add_item(ptrHandle);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::GetPlaybackQueueCount(UINT * outCount)
{
    TRACK_FUNCTION();

    if (!outCount) return E_POINTER;

    (*outCount) = static_api_ptr_t<playlist_manager>()->queue_get_count();
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::GetPlaybackQueueContents(VARIANT * outContents)
{
    TRACK_FUNCTION();

    if (!outContents) return E_POINTER;

    pfc::list_t<t_playback_queue_item> contents;
    helpers::com_array_writer<> arrayWriter;

    static_api_ptr_t<playlist_manager>()->queue_get_contents(contents);
    
    if (!arrayWriter.create(contents.get_count()))
    {
        return E_OUTOFMEMORY;
    }

    for (t_size i = 0; i < contents.get_count(); ++i)
    {
        _variant_t var;
        var.vt = VT_DISPATCH;
        var.pdispVal = new com_object_impl_t<FbPlaybackQueueItem>(contents[i]);

        if (FAILED(arrayWriter.put(i, var)))
        {
            // deep destroy
            arrayWriter.reset();
            return E_OUTOFMEMORY;
        }
    }

    outContents->vt = VT_ARRAY | VT_VARIANT;
    outContents->parray = arrayWriter.get_ptr();
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::FindPlaybackQueueItemIndex(IFbMetadbHandle * handle, UINT playlistIndex, UINT playlistItemIndex, INT * outIndex)
{
    TRACK_FUNCTION();

    if (!outIndex) return E_POINTER;
    if (!handle) return E_INVALIDARG;

    metadb_handle * ptrHandle = NULL;
    handle->get__ptr((void **)&ptrHandle);
    if (!ptrHandle) return E_INVALIDARG;

    t_playback_queue_item item;
    item.m_handle = ptrHandle;
    item.m_playlist = playlistIndex;
    item.m_item = playlistItemIndex;
    (*outIndex) = static_api_ptr_t<playlist_manager>()->queue_find_index(item);
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::FlushPlaybackQueue()
{
    TRACK_FUNCTION();

    static_api_ptr_t<playlist_manager>()->queue_flush();
    return S_OK;
}

STDMETHODIMP FbPlaylistMangerService::IsPlaybackQueueActive(VARIANT_BOOL * outIsActive)
{
    TRACK_FUNCTION();

    if (!outIsActive) return E_POINTER;

    (*outIsActive) = static_api_ptr_t<playlist_manager>()->queue_is_active();
    return S_OK;
}


STDMETHODIMP FbPlaylistManager::GetPlaylistItems(UINT playlistIndex, IFbMetadbHandleList ** outItems)
{
    return FbPlaylistMangerService::GetPlaylistItems(playlistIndex, outItems);
}

STDMETHODIMP FbPlaylistManager::get_PlaybackOrder(UINT * outOrder)
{
    return FbPlaylistMangerService::get_PlaybackOrder(outOrder);
}

STDMETHODIMP FbPlaylistManager::put_PlaybackOrder(UINT order)
{
    return FbPlaylistMangerService::put_PlaybackOrder(order);
}

STDMETHODIMP FbPlaylistManager::get_ActivePlaylist(UINT * outPlaylistIndex)
{
    return FbPlaylistMangerService::get_ActivePlaylist(outPlaylistIndex);
}

STDMETHODIMP FbPlaylistManager::put_ActivePlaylist(UINT playlistIndex)
{
    return FbPlaylistMangerService::put_ActivePlaylist(playlistIndex);
}

STDMETHODIMP FbPlaylistManager::get_PlayingPlaylist(UINT * outPlaylistIndex)
{
    return FbPlaylistMangerService::get_PlayingPlaylist(outPlaylistIndex);
}

STDMETHODIMP FbPlaylistManager::put_PlayingPlaylist(UINT playlistIndex)
{
    return FbPlaylistMangerService::put_PlayingPlaylist(playlistIndex);
}

STDMETHODIMP FbPlaylistManager::get_PlaylistCount(UINT * outCount)
{
    return FbPlaylistMangerService::get_PlaylistCount(outCount);
}

STDMETHODIMP FbPlaylistManager::get_PlaylistItemCount(UINT playlistIndex, UINT * outCount)
{
    return FbPlaylistMangerService::get_PlaylistItemCount(playlistIndex, outCount);
}

STDMETHODIMP FbPlaylistManager::SetPlaylistSelectionSingle(UINT playlistIndex, UINT itemIndex, VARIANT_BOOL state)
{
    return FbPlaylistMangerService::SetPlaylistSelectionSingle(playlistIndex, itemIndex, state);
}

STDMETHODIMP FbPlaylistManager::SetPlaylistSelection(UINT playlistIndex, VARIANT affectedItems, VARIANT_BOOL state)
{
    return FbPlaylistMangerService::SetPlaylistSelection(playlistIndex, affectedItems, state);
}

STDMETHODIMP FbPlaylistManager::ClearPlaylistSelection(UINT playlistIndex)
{
    return FbPlaylistMangerService::ClearPlaylistSelection(playlistIndex);
}

STDMETHODIMP FbPlaylistManager::GetPlaylistFocusItemHandle(VARIANT_BOOL force, IFbMetadbHandle ** outItem)
{
    return FbPlaylistMangerService::GetPlaylistFocusItemHandle(force, outItem);
}

STDMETHODIMP FbPlaylistManager::SetPlaylistFocusItem(UINT playlistIndex, UINT itemIndex)
{
    return FbPlaylistMangerService::SetPlaylistFocusItem(playlistIndex, itemIndex);
}

STDMETHODIMP FbPlaylistManager::SetPlaylistFocusItemByHandle(UINT playlistIndex, IFbMetadbHandle * item)
{
    return FbPlaylistMangerService::SetPlaylistFocusItemByHandle(playlistIndex, item);
}

STDMETHODIMP FbPlaylistManager::GetPlaylistName(UINT playlistIndex, BSTR * outName)
{
    return FbPlaylistMangerService::GetPlaylistName(playlistIndex, outName);
}

STDMETHODIMP FbPlaylistManager::CreatePlaylist(UINT playlistIndex, BSTR name, UINT * outPlaylistIndex)
{
    return FbPlaylistMangerService::CreatePlaylist(playlistIndex, name, outPlaylistIndex);
}

STDMETHODIMP FbPlaylistManager::RemovePlaylist(UINT playlistIndex, VARIANT_BOOL * outSuccess)
{
    return FbPlaylistMangerService::RemovePlaylist(playlistIndex, outSuccess);
}

STDMETHODIMP FbPlaylistManager::MovePlaylist(UINT from, UINT to, VARIANT_BOOL * outSuccess)
{
    return FbPlaylistMangerService::MovePlaylist(from, to, outSuccess);
}

STDMETHODIMP FbPlaylistManager::RenamePlaylist(UINT playlistIndex, BSTR name, VARIANT_BOOL * outSuccess)
{
    return FbPlaylistMangerService::RenamePlaylist(playlistIndex, name, outSuccess);
}

STDMETHODIMP FbPlaylistManager::DuplicatePlaylist(UINT from, BSTR name, UINT * outPlaylistIndex)
{
    return FbPlaylistMangerService::DuplicatePlaylist(from, name, outPlaylistIndex);
}

STDMETHODIMP FbPlaylistManager::CreatePlaybackQueueItem(IFbPlaybackQueueItem ** outPlaybackQueueItem)
{
    return FbPlaylistMangerService::CreatePlaybackQueueItem(outPlaybackQueueItem);
}

STDMETHODIMP FbPlaylistManager::RemoveItemFromPlaybackQueue(UINT index)
{
    return FbPlaylistMangerService::RemoveItemFromPlaybackQueue(index);
}

STDMETHODIMP FbPlaylistManager::RemoveItemsFromPlaybackQueue(VARIANT affectedItems)
{
    return FbPlaylistMangerService::RemoveItemsFromPlaybackQueue(affectedItems);
}

STDMETHODIMP FbPlaylistManager::AddPlaylistItemToPlaybackQueue(UINT playlistIndex, UINT playlistItemIndex)
{
    return FbPlaylistMangerService::AddPlaylistItemToPlaybackQueue(playlistIndex, playlistItemIndex);
}

STDMETHODIMP FbPlaylistManager::AddItemToPlaybackQueue(IFbMetadbHandle * handle)
{
    return FbPlaylistMangerService::AddItemToPlaybackQueue(handle);
}

STDMETHODIMP FbPlaylistManager::GetPlaybackQueueCount(UINT * outCount)
{
    return FbPlaylistMangerService::GetPlaybackQueueCount(outCount);
}

STDMETHODIMP FbPlaylistManager::GetPlaybackQueueContents(VARIANT * outContents)
{
    return FbPlaylistMangerService::GetPlaybackQueueContents(outContents);
}

STDMETHODIMP FbPlaylistManager::FindPlaybackQueueItemIndex(IFbMetadbHandle * handle, UINT playlistIndex, UINT playlistItemIndex, INT * outIndex)
{
    return FbPlaylistMangerService::FindPlaybackQueueItemIndex(handle, playlistIndex, playlistItemIndex, outIndex);
}

STDMETHODIMP FbPlaylistManager::FlushPlaybackQueue()
{
    return FbPlaylistMangerService::FlushPlaybackQueue();
}

STDMETHODIMP FbPlaylistManager::IsPlaybackQueueActive(VARIANT_BOOL * outIsActive)
{
    return FbPlaylistMangerService::IsPlaybackQueueActive(outIsActive);
}


FbPlaybackQueueItem::FbPlaybackQueueItem(const t_playback_queue_item & playbackQueueItem)
{
    m_playback_queue_item.m_handle = playbackQueueItem.m_handle;
    m_playback_queue_item.m_playlist = playbackQueueItem.m_playlist;
    m_playback_queue_item.m_item = playbackQueueItem.m_item;
}

FbPlaybackQueueItem::~FbPlaybackQueueItem()
{

}

void FbPlaybackQueueItem::FinalRelease()
{
    m_playback_queue_item.m_handle.release();
    m_playback_queue_item.m_playlist = 0;
    m_playback_queue_item.m_item = 0;
}

STDMETHODIMP FbPlaybackQueueItem::Equals(IFbPlaybackQueueItem * item, VARIANT_BOOL * outEquals)
{
    TRACK_FUNCTION();

    if (!item) return E_INVALIDARG;

    t_playback_queue_item * ptrQueueItem = NULL;
    item->get__ptr((void **)&ptrQueueItem);
    if (!ptrQueueItem) return E_INVALIDARG;

    (*outEquals) = TO_VARIANT_BOOL(m_playback_queue_item == *ptrQueueItem);
    return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::get__ptr(void ** pp)
{
    TRACK_FUNCTION();

    if (!pp) return E_POINTER;

    (*pp) = &m_playback_queue_item;
    return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::get_Handle(IFbMetadbHandle ** outHandle)
{
    TRACK_FUNCTION();

    if (!outHandle) return E_POINTER;

    (*outHandle) = new com_object_impl_t<FbMetadbHandle>(m_playback_queue_item.m_handle);
    return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::put_Handle(IFbMetadbHandle * handle)
{
    TRACK_FUNCTION();

    if (!handle) return E_INVALIDARG;
    metadb_handle * ptrHandle = NULL;
    handle->get__ptr((void **)&ptrHandle);
    if (!ptrHandle) return E_INVALIDARG;

    m_playback_queue_item.m_handle = ptrHandle;
    return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::get_PlaylistIndex(UINT * outPlaylistIndex)
{
    TRACK_FUNCTION();

    if (!outPlaylistIndex) return E_POINTER;

    (*outPlaylistIndex) = m_playback_queue_item.m_playlist;
    return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::put_PlaylistIndex(UINT playlistIndex)
{
    TRACK_FUNCTION();

    m_playback_queue_item.m_playlist = playlistIndex;
    return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::get_PlaylistItemIndex(UINT * outItemIndex)
{
    TRACK_FUNCTION();

    if (!outItemIndex) return E_POINTER;

    (*outItemIndex) = m_playback_queue_item.m_item;
    return S_OK;
}

STDMETHODIMP FbPlaybackQueueItem::put_PlaylistItemIndex(UINT itemIndex)
{
    TRACK_FUNCTION();

    m_playback_queue_item.m_item = itemIndex;
    return S_OK;
}
