// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        KeymapApplication.cpp
//  Author:      Sandor Vroemisse, Jérôme Duval
//  Description: Keymap Preferences
//  Created :    July 12, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <InputServerDevice.h>
#include <File.h>
#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#if DEBUG
	#include <interface/Input.h>
	#include <stdio.h>
#endif //DEBUG
#include "KeymapWindow.h"
#include "KeymapApplication.h"
#include <stdlib.h>


KeymapApplication::KeymapApplication()
	:	BApplication( APP_SIGNATURE )
{
	// create the window
	BRect frame = WINDOW_DIMENSIONS;
	frame.OffsetTo( WINDOW_LEFT_TOP_POSITION );
	fWindow = new KeymapWindow( frame );
	fWindow->Show();
}

void KeymapApplication::MessageReceived( BMessage * message )
{
	BApplication::MessageReceived( message );
}





/*bool 
KeymapApplication::UseKeymap( BEntry *keymap )
{ // Copies keymap to ~/config/settings/key_map
	BFile	*inFile;
	BFile	*outFile;

	// Open input file
	if( !keymap->Exists() )
		return false;
	inFile = new BFile( keymap, B_READ_ONLY );
	if( !inFile->IsReadable() ) {
		delete inFile;
		return false;
	}
	
	// Is keymap a valid keymap file?
	if( !IsValidKeymap( inFile ) ) {
		delete inFile;
		return false;
	}
	
	// Open output file
	// TODO: find a nice constant for the path and use that instead
	outFile = new BFile( "/boot/home/config/settings/Key_map", B_WRITE_ONLY|B_ERASE_FILE|B_CREATE_FILE );
	if( !outFile->IsWritable() ) {
		delete inFile;
		delete outFile;
		return false;
	}

	// Copy file
	if( Copy( inFile, outFile ) != B_OK ) {
		delete inFile;
		delete outFile;
		return false;
	}
	delete inFile;
	delete outFile;
	
	// Tell Input Server there is a new keymap
	if( NotifyInputServer() != B_OK )
		return false;
	
	return true;
}


bool 
KeymapApplication::IsValidKeymap( BFile *inFile )
{
	// TODO: implement this thing

	return true;
}


int 
KeymapApplication::Copy( BFile *original, BFile *copy )
{
	char	*buffer[ COPY_BUFFER_SIZE ];
	int 	offset = 0;
	int		errorCode = B_NO_ERROR;
	ssize_t	nrBytesRead;
	ssize_t	nrBytesWritten;

	while( true ) {
		nrBytesRead = original->ReadAt( offset, buffer, COPY_BUFFER_SIZE );
		if( nrBytesRead == 0 )
		{
			errorCode = B_OK;
			break;
		}
		nrBytesWritten = copy->WriteAt( offset, buffer, nrBytesRead );
		if( nrBytesWritten != nrBytesRead )
		{
			errorCode = B_ERROR;
			break;
		}
		if( nrBytesRead < COPY_BUFFER_SIZE )
		{
			errorCode = B_OK;
			break;
		}
		offset += nrBytesRead;
	}
	
	return errorCode;
}*/


int 
KeymapApplication::NotifyInputServer()
{
	// This took me days to find out. Go figure.
	// Be didn't need to restart the thing - they prolly used
	// a proprietary interface to notify it. Coordinate with
	// the input_server guys.
	system("kill -KILL input_server");
	return B_NO_ERROR;
}


int 
main ()
{
	new KeymapApplication;
	be_app->Run();
	delete be_app;
	return B_OK;
}
