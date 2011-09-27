#pragma once

#include "script_interface_playlist.h"
#include "com_tools.h"

class FbPlaylistMangerService
{
public:
    // Methods
    static STDMETHODIMP GetPlaylistItems(UINT playlistIndex, IFbMetadbHandleList ** outItems);
    static STDMETHODIMP SetPlaylistSelectionSingle(UINT playlistIndex, UINT itemIndex, VARIANT_BOOL state);
    static STDMETHODIMP SetPlaylistSelection(UINT playlistIndex, VARIANT affectedItems, VARIANT_BOOL state);
    static STDMETHODIMP ClearPlaylistSelection(UINT playlistIndex);
    static STDMETHODIMP GetPlaylistFocusItemHandle(VARIANT_BOOL force, IFbMetadbHandle ** outItem);
    static STDMETHODIMP SetPlaylistFocusItem(UINT playlistIndex, UINT itemIndex);
    static STDMETHODIMP SetPlaylistFocusItemByHandle(UINT playlistIndex, IFbMetadbHandle * item);
    static STDMETHODIMP GetPlaylistName(UINT playlistIndex, BSTR * outName);
    static STDMETHODIMP CreatePlaylist(UINT playlistIndex, BSTR name, UINT * outPlaylistIndex);
    static STDMETHODIMP RemovePlaylist(UINT playlistIndex, VARIANT_BOOL * outSuccess);
    static STDMETHODIMP MovePlaylist(UINT from, UINT to, VARIANT_BOOL * outSuccess);
    static STDMETHODIMP RenamePlaylist(UINT playlistIndex, BSTR name, VARIANT_BOOL * outSuccess);
    static STDMETHODIMP DuplicatePlaylist(UINT from, BSTR name, UINT * outPlaylistIndex);

    static STDMETHODIMP CreatePlaybackQueueItem(IFbPlaybackQueueItem ** outPlaybackQueueItem);
    static STDMETHODIMP RemoveItemFromPlaybackQueue(UINT index);
    static STDMETHODIMP RemoveItemsFromPlaybackQueue(VARIANT affectedItems);
    static STDMETHODIMP AddPlaylistItemToPlaybackQueue(UINT playlistIndex, UINT playlistItemIndex);
    static STDMETHODIMP AddItemToPlaybackQueue(IFbMetadbHandle * handle);
    static STDMETHODIMP GetPlaybackQueueCount(UINT * outCount);
    static STDMETHODIMP GetPlaybackQueueContents(VARIANT * outContents);
    static STDMETHODIMP FindPlaybackQueueItemIndex(IFbMetadbHandle * handle, UINT playlistIndex, UINT playlistItemIndex, INT * outIndex);
    static STDMETHODIMP FlushPlaybackQueue();
    static STDMETHODIMP IsPlaybackQueueActive(VARIANT_BOOL * outIsActive);

    // Properties
    static STDMETHODIMP get_PlaybackOrder(UINT * outOrder);
    static STDMETHODIMP put_PlaybackOrder(UINT order);
    static STDMETHODIMP get_ActivePlaylist(UINT * outPlaylistIndex);
    static STDMETHODIMP put_ActivePlaylist(UINT playlistIndex);
    static STDMETHODIMP get_PlayingPlaylist(UINT * outPlaylistIndex);
    static STDMETHODIMP put_PlayingPlaylist(UINT playlistIndex);
    static STDMETHODIMP get_PlaylistCount(UINT * outCount);
    static STDMETHODIMP get_PlaylistItemCount(UINT playlistIndex, UINT * outCount);
};

// NOTE: Do not use com_object_impl_t<> to initialize, use com_object_singleton_t<> instead.
class FbPlaylistManager : public IDispatchImpl3<IFbPlaylistManager>
{
public:
    // Methods
    STDMETHODIMP GetPlaylistItems(UINT playlistIndex, IFbMetadbHandleList ** outItems);
    STDMETHODIMP SetPlaylistSelectionSingle(UINT playlistIndex, UINT itemIndex, VARIANT_BOOL state);
    STDMETHODIMP SetPlaylistSelection(UINT playlistIndex, VARIANT affectedItems, VARIANT_BOOL state);
    STDMETHODIMP ClearPlaylistSelection(UINT playlistIndex);
    STDMETHODIMP GetPlaylistFocusItemHandle(VARIANT_BOOL force, IFbMetadbHandle ** outItem);
    STDMETHODIMP SetPlaylistFocusItem(UINT playlistIndex, UINT itemIndex);
    STDMETHODIMP SetPlaylistFocusItemByHandle(UINT playlistIndex, IFbMetadbHandle * item);
    STDMETHODIMP GetPlaylistName(UINT playlistIndex, BSTR * outName);
    STDMETHODIMP CreatePlaylist(UINT playlistIndex, BSTR name, UINT * outPlaylistIndex);
    STDMETHODIMP RemovePlaylist(UINT playlistIndex, VARIANT_BOOL * outSuccess);
    STDMETHODIMP MovePlaylist(UINT from, UINT to, VARIANT_BOOL * outSuccess);
    STDMETHODIMP RenamePlaylist(UINT playlistIndex, BSTR name, VARIANT_BOOL * outSuccess);
    STDMETHODIMP DuplicatePlaylist(UINT from, BSTR name, UINT * outPlaylistIndex);

    STDMETHODIMP CreatePlaybackQueueItem(IFbPlaybackQueueItem ** outPlaybackQueueItem);
    STDMETHODIMP RemoveItemFromPlaybackQueue(UINT index);
    STDMETHODIMP RemoveItemsFromPlaybackQueue(VARIANT affectedItems);
    STDMETHODIMP AddPlaylistItemToPlaybackQueue(UINT playlistIndex, UINT playlistItemIndex);
    STDMETHODIMP AddItemToPlaybackQueue(IFbMetadbHandle * handle);
    STDMETHODIMP GetPlaybackQueueCount(UINT * outCount);
    STDMETHODIMP GetPlaybackQueueContents(VARIANT * outContents);
    STDMETHODIMP FindPlaybackQueueItemIndex(IFbMetadbHandle * handle, UINT playlistIndex, UINT playlistItemIndex, INT * outIndex);
    STDMETHODIMP FlushPlaybackQueue();
    STDMETHODIMP IsPlaybackQueueActive(VARIANT_BOOL * outIsActive);

    // Properties
    STDMETHODIMP get_PlaybackOrder(UINT * outOrder);
    STDMETHODIMP put_PlaybackOrder(UINT order);
    STDMETHODIMP get_ActivePlaylist(UINT * outPlaylistIndex);
    STDMETHODIMP put_ActivePlaylist(UINT playlistIndex);
    STDMETHODIMP get_PlayingPlaylist(UINT * outPlaylistIndex);
    STDMETHODIMP put_PlayingPlaylist(UINT playlistIndex);
    STDMETHODIMP get_PlaylistCount(UINT * outCount);
    STDMETHODIMP get_PlaylistItemCount(UINT playlistIndex, UINT * outCount);
};

class FbPlaybackQueueItem : public IDisposableImpl4<IFbPlaybackQueueItem>
{
protected:
    t_playback_queue_item m_playback_queue_item;

    FbPlaybackQueueItem() {}
    FbPlaybackQueueItem(const t_playback_queue_item & playbackQueueItem);

    virtual ~FbPlaybackQueueItem();

    virtual void FinalRelease();

public:
    // Methods
    STDMETHODIMP Equals(IFbPlaybackQueueItem * item, VARIANT_BOOL * outEquals);

    // Properties
    STDMETHODIMP get__ptr(void ** pp);
    STDMETHODIMP get_Handle(IFbMetadbHandle ** outHandle);
    STDMETHODIMP put_Handle(IFbMetadbHandle * handle);
    STDMETHODIMP get_PlaylistIndex(UINT * outPlaylistIndex);
    STDMETHODIMP put_PlaylistIndex(UINT playlistIndex);
    STDMETHODIMP get_PlaylistItemIndex(UINT * outItemIndex);
    STDMETHODIMP put_PlaylistItemIndex(UINT itemIndex);
};
