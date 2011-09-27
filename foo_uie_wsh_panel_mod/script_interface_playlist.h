#pragma once

#include <ObjBase.h>

// forward declarations
__interface IFbMetadbHandleList;

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