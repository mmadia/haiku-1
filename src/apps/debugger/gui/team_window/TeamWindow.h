/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_WINDOW_H
#define TEAM_WINDOW_H

#include <String.h>
#include <Window.h>


class BTabView;
class ImageListView;
class Team;
class ThreadListView;


class TeamWindow : public BWindow {
public:
	class Listener;

public:
								TeamWindow(::Team* team, Listener* listener);
								~TeamWindow();

	static	TeamWindow*			Create(::Team* team, Listener* listener);
									// throws

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			void				_Init();

private:
			::Team*				fTeam;
			Listener*			fListener;
			BTabView*			fTabView;
			ThreadListView*		fThreadListView;
			ImageListView*		fImageListView;
};


class TeamWindow::Listener {
public:
	virtual						~Listener();

	virtual	bool				TeamWindowQuitRequested(TeamWindow* window);
};


#endif	// TEAM_WINDOW_H
