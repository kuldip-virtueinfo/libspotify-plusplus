/*
 * Copyright 2011 Jim Knowler
 *           2012 Alexander Rojas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// c-lib includes
#include <assert.h>

// local includes
#include "spotify/PlayListContainer.hpp"
#include "spotify/PlayListFolder.hpp"
#include "spotify/Session.hpp"

// debugging
#include "debug/Debug.hpp"
#define LOG( msg, ... ) //Debug::PrintLine( msg, __VA_ARGS__ );

namespace spotify {
PlayListContainer::PlayListContainer(boost::shared_ptr<Session> session) : PlayListElement(session), pContainer_(NULL), isLoading_(false) {
}

PlayListContainer::~PlayListContainer() {
    Unload();
}

PlayListElement::eType PlayListContainer::GetType() {
    return PLAYLIST_CONTAINER;
}

void PlayListContainer::GetCallbacks(sp_playlistcontainer_callbacks &callbacks) {
    memset(&callbacks, 0, sizeof(callbacks));

    callbacks.container_loaded = callback_container_loaded;
    callbacks.playlist_added = callback_playlist_added;
    callbacks.playlist_moved = callback_playlist_moved;
    callbacks.playlist_removed = callback_playlist_removed;
}

bool PlayListContainer::Load(sp_playlistcontainer *container) {
    pContainer_ = container;

    sp_playlistcontainer_callbacks callbacks;
    GetCallbacks(callbacks);

    sp_playlistcontainer_add_callbacks(pContainer_, &callbacks, this);

    isLoading_ = true;

    return true;
}

void PlayListContainer::Unload() {
    if (pContainer_) {
        sp_playlistcontainer_callbacks callbacks;
        GetCallbacks(callbacks);

        sp_playlistcontainer_remove_callbacks(pContainer_, &callbacks, this);

        pContainer_ = NULL;

        playLists_.clear();

        isLoading_ = false;
    }
}

bool PlayListContainer::IsLoading(bool recursive) {
    if (isLoading_) {
        return true;
    }

    if (recursive) {
        int numPlayLists = GetNumChildren();

        for (int i = 0; i < numPlayLists; i++) {
            if (playLists_[i]->IsLoading(recursive)) {
                return true;
            }
        }
    }

    return false;
}

void PlayListContainer::AddPlayList(boost::shared_ptr<PlayListElement> playList) {
    playList->SetParent(shared_from_this());
    playLists_.push_back(playList);
}

bool PlayListContainer::HasChildren() {
    return !playLists_.empty();
}

int PlayListContainer::GetNumChildren() {
    return playLists_.size();
}

boost::shared_ptr<PlayListElement> PlayListContainer::GetChild(int index) {
    return playLists_[index];
}

void PlayListContainer::DumpToTTY(int level) {
    debug::PrintLine(level, "PlayListContainer");

    level ++;

    int numPlayLists = GetNumChildren();

    for (int i = 0; i < numPlayLists; i++) {
        GetChild(i)->DumpToTTY(level);
    }
}

std::string PlayListContainer::GetName() {
    return "Container";
}

PlayListContainer *PlayListContainer::GetPlayListContainer(sp_playlistcontainer *pc, void *userdata) {
    PlayListContainer *pContainer = reinterpret_cast<PlayListContainer *>(userdata);
    assert(pContainer->pContainer_ == pc);

    return pContainer;
}

void PlayListContainer::callback_playlist_added(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata) {
    PlayListContainer *pContainer = GetPlayListContainer(pc, userdata);
    pContainer->OnPlaylistAdded(playlist, position);
}

void PlayListContainer::callback_playlist_removed(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata) {
    PlayListContainer *pContainer = GetPlayListContainer(pc, userdata);
    pContainer->OnPlaylistRemoved(playlist, position);
}

void PlayListContainer::callback_playlist_moved(sp_playlistcontainer *pc, sp_playlist *playlist, int position, int new_position, void *userdata) {
    PlayListContainer *pContainer = GetPlayListContainer(pc, userdata);
    pContainer->OnPlaylistMoved(playlist, position, new_position);
}

void PlayListContainer::callback_container_loaded(sp_playlistcontainer *pc, void *userdata) {
    PlayListContainer *pContainer = GetPlayListContainer(pc, userdata);

    pContainer->isLoading_ = false;

    pContainer->OnContainerLoaded();
}

void PlayListContainer::OnPlaylistAdded(sp_playlist *playlist, int position) {
    LOG("OnPlaylistAdded [0x%08X]", playlist);
}

void PlayListContainer::OnPlaylistRemoved(sp_playlist *playlist, int position) {
    LOG("OnPlaylistRemoved [0x%08X]", playlist);
}

void PlayListContainer::OnPlaylistMoved(sp_playlist *playlist, int position, int newPosition) {
    LOG("OnPlaylistMoved [0x%08X]", playlist);
}

void PlayListContainer::OnContainerLoaded() {
    LOG("OnContainerLoaded");

    int numPlaylists = sp_playlistcontainer_num_playlists(pContainer_);

    boost::shared_ptr<PlayListElement> itContainer = shared_from_this();

    for (int i = 0; (i < numPlaylists); i++) {
        sp_playlist_type type = sp_playlistcontainer_playlist_type(pContainer_, i);

        switch (type) {
            case SP_PLAYLIST_TYPE_PLAYLIST: {
                sp_playlist *p = sp_playlistcontainer_playlist(pContainer_, i);

                boost::shared_ptr<PlayList> playList = session_->CreatePlayList();
                playList->Load(p);

                itContainer->AddPlayList(playList);
            }
            break;

            case SP_PLAYLIST_TYPE_START_FOLDER: {
                boost::shared_ptr<PlayListFolder> folder = session_->CreatePlayListFolder();
                folder->Load(pContainer_, i);

                itContainer->AddPlayList(folder);
                itContainer = folder;
            }
            break;

            case SP_PLAYLIST_TYPE_END_FOLDER:
                itContainer = itContainer->GetParent();
                break;

            case SP_PLAYLIST_TYPE_PLACEHOLDER:
            default:
                LOG("OTHER???");

                // ??
                break;
        }

    }

    assert(itContainer == shared_from_this());

}
}