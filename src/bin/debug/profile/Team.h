/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_H
#define TEAM_H

#include <debug_support.h>
#include <ObjectList.h>
#include <util/DoublyLinkedList.h>

#include "Thread.h"


struct system_profiler_team_added;


class Team {
public:
								Team();
								~Team();

			status_t			Init(team_id teamID, port_id debuggerPort);
			status_t			Init(system_profiler_team_added* addedInfo);
			status_t			InitThread(Thread* thread);

			void				RemoveThread(Thread* thread);

			void				Exec(int32 event);

			status_t			AddImage(SharedImage* sharedImage,
									const image_info& imageInfo, team_id owner,
									int32 event);
			status_t			RemoveImage(image_id imageID, int32 event);

	inline	const BObjectList<Image>&	Images() const;
			Image*				FindImage(image_id id) const;

	inline	team_id				ID() const;

private:
			void				_RemoveImage(int32 index, int32 event);

			bool				_SynchronousProfiling() const
									{ return fDebugContext.nub_port < 0; }

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
