/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <TextControl.h>
#include "TTextControl.h"

/************************************************************************
 *
 * CONSTRUCTOR and DESTRUCTOR
 *
 ***********************************************************************/

////////////////////////////////////////////////////////////////////////////
// TTextControl ()
//	Constructor.
////////////////////////////////////////////////////////////////////////////
TTextControl::TTextControl (BRect r, const char *name,
			    const char *label, const char *text,
			    BMessage *msg)
  :BTextControl (r, name, label, text, msg)
{
  mModified = false;
}

////////////////////////////////////////////////////////////////////////////
// ~TTextControl ()
//	Destructor.
////////////////////////////////////////////////////////////////////////////
TTextControl::~TTextControl ()
{
}

/*
 * PUBLIC MEMBER FUNCTIONS.
 */

////////////////////////////////////////////////////////////////////////////
// ModifiedText (int)
//	Dispatch modification message.
////////////////////////////////////////////////////////////////////////////
void
TTextControl::ModifiedText (int flag)
{
  mModified = flag;
  return;
}

////////////////////////////////////////////////////////////////////////////
// IsModified (void)
//	Dispatch modification message.
////////////////////////////////////////////////////////////////////////////
int
TTextControl::IsModified (void)
{
  return mModified;
}

