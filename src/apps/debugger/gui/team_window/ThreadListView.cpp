/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ThreadListView.h"

#include <stdio.h>

#include <new>

#include <Looper.h>
#include <Message.h>

#include <AutoLocker.h>
#include <ObjectList.h>

#include "table/TableColumns.h"


enum {
	MSG_SYNC_THREAD_LIST	= 'sytl'
};


// #pragma mark - ThreadsTableModel


class ThreadListView::ThreadsTableModel : public TableModel {
public:
	ThreadsTableModel(Team* team)
		:
		fTeam(team)
	{
		Update();
	}

	~ThreadsTableModel()
	{
		fTeam = NULL;
		Update();
	}

	bool Update()
	{
		for (int32 i = 0; Thread* thread = fThreads.ItemAt(i); i++)
			thread->RemoveReference();
		fThreads.MakeEmpty();

		if (fTeam == NULL)
			return true;

		AutoLocker<Team> locker(fTeam);

		for (ThreadList::ConstIterator it = fTeam->Threads().GetIterator();
				Thread* thread = it.Next();) {
			if (!fThreads.AddItem(thread))
				return false;

			thread->AddReference();
		}

		return true;
	}

	virtual int32 CountColumns() const
	{
		return 2;
	}

	virtual int32 CountRows() const
	{
		return fThreads.CountItems();
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, Variant& value)
	{
		Thread* thread = fThreads.ItemAt(rowIndex);
		if (thread == NULL)
			return false;

		switch (columnIndex) {
			case 0:
				value.SetTo(thread->ID());
				return true;
			case 1:
				value.SetTo(thread->Name(), VARIANT_DONT_COPY_DATA);
				return true;
			default:
				return false;
		}
	}

	Thread* ThreadAt(int32 index) const
	{
		return fThreads.ItemAt(index);
	}

private:
	Team*				fTeam;
	BObjectList<Thread>	fThreads;
};


// #pragma mark - ThreadListView


ThreadListView::ThreadListView()
	:
	BGroupView(B_VERTICAL),
	fTeam(NULL),
	fThreadsTable(NULL),
	fThreadsTableModel(NULL)
{
	SetName("Threads");
}


ThreadListView::~ThreadListView()
{
	SetTeam(NULL);
	fThreadsTable->SetTableModel(NULL);
	delete fThreadsTableModel;
}


/*static*/ ThreadListView*
ThreadListView::Create()
{
	ThreadListView* self = new ThreadListView;

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
ThreadListView::SetTeam(Team* team)
{
	if (team == fTeam)
		return;

	if (fTeam != NULL) {
		fTeam->RemoveListener(this);
		fThreadsTable->SetTableModel(NULL);
		delete fThreadsTableModel;
		fThreadsTableModel = NULL;
	}

	fTeam = team;

	if (fTeam != NULL) {
		fThreadsTableModel = new(std::nothrow) ThreadsTableModel(fTeam);
		fThreadsTable->SetTableModel(fThreadsTableModel);
		fThreadsTable->ResizeAllColumnsToPreferred();
		fTeam->AddListener(this);
	}
}


void
ThreadListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SYNC_THREAD_LIST:
			if (fThreadsTableModel != NULL) {
				fThreadsTable->SetTableModel(NULL);
				fThreadsTableModel->Update();
				fThreadsTable->SetTableModel(fThreadsTableModel);
			}
			break;
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
ThreadListView::ThreadAdded(Team* team, Thread* thread)
{
	Looper()->PostMessage(MSG_SYNC_THREAD_LIST, this);
}


void
ThreadListView::ThreadRemoved(Team* team, Thread* thread)
{
	Looper()->PostMessage(MSG_SYNC_THREAD_LIST, this);
}


void
ThreadListView::TableRowInvoked(Table* table, int32 rowIndex)
{
//	if (fThreadsTableModel != NULL) {
//		Thread* thread = fThreadsTableModel->ThreadAt(rowIndex);
//		if (thread != NULL)
//			fParent->OpenThreadWindow(thread);
//	}
}


void
ThreadListView::_Init()
{
	fThreadsTable = new Table("threads list", 0);
	AddChild(fThreadsTable->ToView());
	
	// columns
	fThreadsTable->AddColumn(new Int32TableColumn(0, "ID", 40, 20, 1000,
		B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new StringTableColumn(1, "Name", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	
	fThreadsTable->AddTableListener(this);
}
