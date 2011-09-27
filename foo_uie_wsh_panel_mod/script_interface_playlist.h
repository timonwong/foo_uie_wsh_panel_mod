#pragma once

#include <ObjBase.h>

// forward declarations
__interface IFbMetadbHandleList;
__interface IFbPlaybackQueueItem;

//---
[
    object,
    dual,
    pointer_default(unique),
    library_block,
    uuid("84212840-c0c5-4625-8fc4-2cc20e4bbcc8")
]
__interface IFbPlaylistManager : IDispatch
{
    // Methods
    STDMETHOD(GetPlaylistItems)(UINT playlistIndex, [out,retval] IFbMetadbHandleList ** outItems);
    STDMETHOD(SetPlaylistSelectionSingle)(UINT playlistIndex, UINT itemIndex, VARIANT_BOOL state);
    STDMETHOD(SetPlaylistSelection)(UINT playlistIndex, VARIANT affectedItems, VARIANT_BOOL state);
    STDMETHOD(ClearPlaylistSelection)(UINT playlistIndex);
    STDMETHOD(GetPlaylistFocusItemHandle)(VARIANT_BOOL force, [out,retval] IFbMetadbHandle ** outItem);
    STDMETHOD(SetPlaylistFocusItem)(UINT playlistIndex, UINT itemIndex);
    STDMETHOD(SetPlaylistFocusItemByHandle)(UINT playlistIndex, IFbMetadbHandle * item);
    STDMETHOD(GetPlaylistName)(UINT playlistIndex, [out,retval] BSTR * outName);
    STDMETHOD(CreatePlaylist)(UINT playlistIndex, BSTR name, [out,retval] UINT * outPlaylistIndex);
    STDMETHOD(RemovePlaylist)(UINT playlistIndex, [out,retval] VARIANT_BOOL * outSuccess);
    STDMETHOD(MovePlaylist)(UINT from, UINT to, [out,retval] VARIANT_BOOL * outSuccess);
    STDMETHOD(RenamePlaylist)(UINT playlistIndex, BSTR name, [out,retval] VARIANT_BOOL * outSuccess);
    STDMETHOD(DuplicatePlaylist)(UINT from, BSTR name, [out,retval] UINT * outPlaylistIndex);

    STDMETHOD(RemoveItemFromPlaybackQueue)(UINT index);
    STDMETHOD(RemoveItemsFromPlaybackQueue)(VARIANT affectedItems);
    STDMETHOD(AddPlaylistItemToPlaybackQueue)(UINT playlistIndex, UINT playlistItemIndex);
    STDMETHOD(AddItemToPlaybackQueue)(IFbMetadbHandle * handle);
    STDMETHOD(GetPlaybackQueueCount)([out,retval] UINT * outCount);
    STDMETHOD(GetPlaybackQueueContents)([out,retval] VARIANT * outContents);
    STDMETHOD(FindPlaybackQueueItemIndex)(IFbMetadbHandle * handle, UINT playlistIndex, UINT playlistItemIndex, [out,retval] INT * outIndex);
    STDMETHOD(FlushPlaybackQueue)();
    STDMETHOD(IsPlaybackQueueActive)([out,retval] VARIANT_BOOL * outIsActive);

    // Properties
    [propget] STDMETHOD(PlaybackOrder)([out,retval] UINT * outOrder);
    [propput] STDMETHOD(PlaybackOrder)(UINT order);
    [propget] STDMETHOD(ActivePlaylist)([out,retval] UINT * outPlaylistIndex);
    [propput] STDMETHOD(ActivePlaylist)(UINT playlistIndex);
    [propget] STDMETHOD(PlayingPlaylist)([out,retval] UINT * outPlaylistIndex);
    [propput] STDMETHOD(PlayingPlaylist)(UINT playlistIndex);
    [propget] STDMETHOD(PlaylistCount)([out,retval] UINT * outCount);
    [propget] STDMETHOD(PlaylistItemCount)(UINT playlistIndex, [out,retval] UINT * outCount);
};
_COM_SMARTPTR_TYPEDEF(IFbPlaylistManager, __uuidof(IFbPlaylistManager));

[
    object,
    dual,
    pointer_default(unique),
    library_block,
    uuid("e6d4354c-9a79-4062-b4d7-714b13539500")
]
__interface IFbPlaybackQueueItem : IDisposable
{
    // Methods
    STDMETHOD(Equals)(IFbPlaybackQueueItem * item, [out,retval] VARIANT_BOOL * outEquals);

    // Properties
    [propget] STDMETHOD(_ptr)([out,retval] void ** pp);
    [propget] STDMETHOD(Handle)([out,retval] IFbMetadbHandle ** outHandle);
    [propput] STDMETHOD(Handle)(IFbMetadbHandle * handle);
    [propget] STDMETHOD(PlaylistIndex)([out,retval] UINT * outPlaylistIndex);
    [propput] STDMETHOD(PlaylistIndex)(UINT playlistIndex);
    [propget] STDMETHOD(PlaylistItemIndex)([out,retval] UINT * outItemIndex);
    [propput] STDMETHOD(PlaylistItemIndex)(UINT itemIndex);
};
