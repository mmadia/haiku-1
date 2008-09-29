/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_H
#define TEAM_H

#include <debug_support.h>
#include <ObjectList.h>
#include <util/DoublyLinkedList.h>

#include "Thread.h"


class Team {
public:
								Team();
								~Team();

			status_t			Init(team_id teamID, port_id debuggerPort);
			status_t			InitThread(Thread* thread);

			void				RemoveThread(Thread* thread);

			void				Exec(int32 event);

			status_t			AddImage(const image_info& imageInfo,
									int32 event);
			status_t			RemoveImage(const image_info& imageInfo,
									int32 event);

	inline	const BObjectList<Image>&	Images() const;
			Image*				FindImage(image_id id) const;

	inline	team_id				ID() const;

private:
			status_t			_LoadSymbols(
									debug_symbol_lookup_context* lookupContext,
									team_id owner);
			status_t			_LoadImageSymbols(
									debug_symbol_lookup_context* lookupContext,
									const image_info& imageInfo, team_id owner,
									int32 event, Image** _image = NULL);
			void				_RemoveImage(int32 index, int32 event);

private:
	typedef DoublyLinkedList<Thread> ThreadList;

			team_info			fInfo;
			port_id				fNubPort;
			debug_context		fDebugContext;
			ThreadList			fThreads;
			BObjectList<Image>	fImages;
};


// #pragma mark -


const BObjectList<Image>&
Team::Images() const
{
	return fImages;
}


team_id
Team::ID() const
{
	return fInfo.team;
}


#endif	// TEAM_H
