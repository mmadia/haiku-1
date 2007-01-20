/*
 * Copyright 2003-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin	
 *		Michael Pfeiffer
 *		Dr. Hartmut Reh
 */

#include <stdio.h>
#include <string.h>			// for memset()

#include <StorageKit.h>

#include "PrinterDriver.h"

#include "PrinterSetupWindow.h"
#include "PageSetupWindow.h"
#include "JobSetupWindow.h"

// Private prototypes
// ------------------

#ifdef CODEWARRIOR
	#pragma mark [Constructor & destructor]
#endif

// Constructor & destructor
// ------------------------

// --------------------------------------------------
PrinterDriver::PrinterDriver(BNode* printerNode)
	:	fJobFile(NULL),
		fPrinterNode(printerNode),
		fJobMsg(NULL)
{
}


// --------------------------------------------------
PrinterDriver::~PrinterDriver() 
{
}

#ifdef CODEWARRIOR
	#pragma mark [Public methods]
#endif

#ifdef B_BEOS_VERSION_DANO
struct print_file_header {
       int32   version;
       int32   page_count;
       off_t   first_page;
       int32   _reserved_3_;
       int32   _reserved_4_;
       int32   _reserved_5_;
};
#endif


// Public methods
// --------------

status_t 
PrinterDriver::PrintJob
	(
	BFile 		*jobFile,		// spool file
	BMessage 	*jobMsg			// job message
	)
{
	print_file_header	pfh;
	status_t			status;
	BMessage 			*msg;
	int32 				page;
	uint32				copy;
	uint32				copies;
	const int32         passes = 2;

	fJobFile		= jobFile;
	fJobMsg			= jobMsg;

	if (!fJobFile || !fPrinterNode) 
		return B_ERROR;

	// read print file header	
	fJobFile->Seek(0, SEEK_SET);
	fJobFile->Read(&pfh, sizeof(pfh));
	
	// read job message
	fJobMsg = msg = new BMessage();
	msg->Unflatten(fJobFile);
	
	if (msg->HasInt32("copies")) {
		copies = msg->FindInt32("copies");
	} else {
		copies = 1;
	}
	
	status = BeginJob();

	fPrinting = true;
	for (fPass = 0; fPass < passes && status == B_OK && fPrinting; fPass++) {
		for (copy = 0; copy < copies && status == B_OK && fPrinting; copy++) 
		{
			for (page = 1; page <= pfh.page_count && status == B_OK && fPrinting; page++) {
				status = PrintPage(page, pfh.page_count);
			}
	
			// re-read job message for next page
			fJobFile->Seek(sizeof(pfh), SEEK_SET);
			msg->Unflatten(fJobFile);
		}
	}
	
	status_t s = EndJob();
	if (status == B_OK) status = s;
		
	delete fJobMsg;
		
	return status;
}

/**
 * This will stop the printing loop
 *
 * @param none
 * @return void
 */
void 
PrinterDriver::StopPrinting()
{
	fPrinting = false;
}


// --------------------------------------------------
status_t
PrinterDriver::BeginJob() 
{
	return B_OK;
}


// --------------------------------------------------
status_t 
PrinterDriver::PrintPage(int32 pageNumber, int32 pageCount) 
{
	char text[128];

	sprintf(text, "Faking print of page %ld/%ld...", pageNumber, pageCount);
	BAlert *alert = new BAlert("PrinterDriver::PrintPage()", text, "Hmm?");
	alert->Go();
	return B_OK;
}


// --------------------------------------------------
status_t
PrinterDriver::EndJob() 
{
	return B_OK;
}


BlockingWindow* PrinterDriver::NewPrinterSetupWindow(char* printerName) {
	return NULL;
}

BlockingWindow* PrinterDriver::NewPageSetupWindow(BMessage *setupMsg, const char *printerName) {
	return new PageSetupWindow(setupMsg, printerName);
}

BlockingWindow* PrinterDriver::NewJobSetupWindow(BMessage *jobMsg, const char *printerName) {
	return new JobSetupWindow(jobMsg, printerName);
}

status_t PrinterDriver::Go(BlockingWindow* w) {
	if (w) {
		return w->Go();
	} else {
		return B_OK;
	}
}

// --------------------------------------------------
status_t 
PrinterDriver::PrinterSetup(char *printerName)
	// name of printer, to attach printer settings
{
	return Go(NewPrinterSetupWindow(printerName));
}


// --------------------------------------------------
status_t 
PrinterDriver::PageSetup(BMessage *setupMsg, const char *printerName)
{
	// check to see if the messag is built correctly...
	if (setupMsg->HasFloat("scaling") != B_OK) {
#if HAS_PRINTER_SETTINGS
		PrinterSettings *ps = new PrinterSettings(printerName);

		if (ps->InitCheck() == B_OK) {
			// first read the settings from the spool dir
			if (ps->ReadSettings(setupMsg) != B_OK) {
				// if there were none, then create a default set...
				ps->GetDefaults(setupMsg);
				// ...and save them
				ps->WriteSettings(setupMsg);
			}
		}			
#endif
	}

	return Go(NewPageSetupWindow(setupMsg, printerName));
}


// --------------------------------------------------
status_t 
PrinterDriver::JobSetup(BMessage *jobMsg, const char *printerName)
{
	// set default value if property not set
	if (!jobMsg->HasInt32("copies"))
		jobMsg->AddInt32("copies", 1);

	if (!jobMsg->HasInt32("first_page"))
		jobMsg->AddInt32("first_page", 1);
		
	if (!jobMsg->HasInt32("last_page"))
		jobMsg->AddInt32("last_page", MAX_INT32);

	return Go(NewJobSetupWindow(jobMsg, printerName));
}

// --------------------------------------------------
BMessage*       
PrinterDriver::GetDefaultSettings() 
{
	BMessage* msg = new BMessage();
	BRect paperRect(0, 0, letter_width, letter_height);
	BRect printableRect(paperRect);
	printableRect.InsetBy(10, 10);
	msg->AddRect("paper_rect", paperRect);
	msg->AddRect("printable_rect", printableRect);
	msg->AddInt32("orientation", 0);
	msg->AddInt32("xres", 300);
	msg->AddInt32("yres", 300);
	return msg;
}

#ifdef CODEWARRIOR
	#pragma mark [Privates routines]
#endif

// Private routines
// ----------------
