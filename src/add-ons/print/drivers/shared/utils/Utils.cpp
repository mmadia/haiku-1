/*

Preview printer driver.

Copyright (c) 2001, 2002 OpenBeOS.
Copyright (c) 2005 - 2008 Haiku.

Authors:
	Philippe Houdoin
	Simon Gauvin
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include "Utils.h"


#include <Message.h>
#include <Window.h>


void
SetBool(BMessage* msg, const char* name, bool value)
{
	if (msg->HasBool(name)) {
		msg->ReplaceBool(name, value);
	} else {
		msg->AddBool(name, value);
	}
}


void
SetFloat(BMessage* msg, const char* name, float value)
{
	if (msg->HasFloat(name)) {
		msg->ReplaceFloat(name, value);
	} else {
		msg->AddFloat(name, value);
	}
}


void
SetInt32(BMessage* msg, const char* name, int32 value)
{
	if (msg->HasInt32(name)) {
		msg->ReplaceInt32(name, value);
	} else {
		msg->AddInt32(name, value);
	}
}


void
SetString(BMessage* msg, const char* name, const char* value)
{
	if (msg->HasString(name, 0)) {
		msg->ReplaceString(name, value);
	} else {
		msg->AddString(name, value);
	}
}


void
SetRect(BMessage* msg, const char* name, const BRect& rect)
{
	if (msg->HasRect(name)) {
		msg->ReplaceRect(name, rect);
	} else {
		msg->AddRect(name, rect);
	}
}


void
SetString(BMessage* msg, const char* name, const BString& value)
{
	SetString(msg, name, value.String());
}


// #pragma mark -- EscapeMessageFilter


EscapeMessageFilter::EscapeMessageFilter(BWindow *window, int32 what)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, '_KYD')
	, fWindow(window),
	fWhat(what)
{
}


filter_result
EscapeMessageFilter::Filter(BMessage *msg, BHandler **target)
{
	int32 key;
	// notify window with message fWhat if Escape key is hit
	if (B_OK == msg->FindInt32("key", &key) && key == 1) {
		fWindow->PostMessage(fWhat);
		return B_SKIP_MESSAGE;
	}
	return B_DISPATCH_MESSAGE;
}
