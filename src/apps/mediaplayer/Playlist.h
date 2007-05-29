/*
 * Playlist.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef __PLAYLIST_H
#define __PLAYLIST_H

#include <Entry.h>
#include <List.h>

class Playlist {
public:
	class Listener {
	public:
						Listener();
		virtual			~Listener();

		virtual	void	RefAdded(const entry_ref& ref, int32 index);
		virtual	void	RefRemoved(int32 index);

		virtual	void	RefsSorted();

		virtual	void	CurrentRefChanged(int32 newIndex);
	};

public:
						Playlist();
						~Playlist();
					
	// list functionality
	void				MakeEmpty();
	int32				CountItems() const;
	
	void				Sort();

	bool				AddRef(const entry_ref& ref);
	bool				AddRef(const entry_ref& ref, int32 index);
	entry_ref			RemoveRef(int32 index);

	bool				AdoptPlaylist(Playlist& other);
	bool				AdoptPlaylist(Playlist& other, int32 index);

//	int32				IndexOf(const entry_ref& ref) const;
	status_t			GetRefAt(int32 index, entry_ref* ref) const;
//	bool				HasRef(const entry_ref& ref) const;

	// navigating current ref
	// TODO: replace with SetCurrentRef() and listener
//	status_t			NextRef(entry_ref* ref);
//	status_t			PrevRef(entry_ref* ref);

	void				SetCurrentRefIndex(int32 index);
	int32				CurrentRefIndex() const;

	void				GetSkipInfo(bool* canSkipPrevious,
							bool* canSkipNext) const;

	// listener support
	bool				AddListener(Listener* listener);
	void				RemoveListener(Listener* listener);

private:
	static int			playlist_cmp(const void* p1, const void* p2);

	void				_NotifyRefAdded(const entry_ref& ref,
							int32 index) const;
	void				_NotifyRefRemoved(int32 index) const;
	void				_NotifyRefsSorted() const;
	void				_NotifyCurrentRefChanged(int32 newIndex) const;

private:
	BList				fRefs;
	BList				fListeners;

	int32				fCurrentIndex;
};

#endif
