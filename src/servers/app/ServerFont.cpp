/*
 * Copyright 2001-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Jérôme Duval, jerome.duval@free.fr
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "ServerFont.h"

#include "Angle.h"
#include "GlyphLayoutEngine.h"
#include "FontManager.h"
#include "truncate_string.h"
#include "utf8_functions.h"

#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include <Shape.h>
#include <String.h>
#include <UTF8.h>

#include <agg_bounding_rect.h>

#include <stdio.h>
#include <string.h>

// functions needed to convert a freetype vector graphics to a BShape
inline BPoint
VectorToPoint(const FT_Vector *vector)
{
	BPoint result;
	result.x = float(vector->x) / 64;
	result.y = -float(vector->y) / 64;
	return result;
}


int
MoveToFunc(const FT_Vector *to, void *user)
{
	((BShape *)user)->MoveTo(VectorToPoint(to));
	return 0;
}


int
LineToFunc(const FT_Vector *to, void *user)
{
	((BShape *)user)->LineTo(VectorToPoint(to));
	return 0;
}


int
ConicToFunc(const FT_Vector *control, const FT_Vector *to, void *user)
{
	BPoint controls[3];

	controls[0] = VectorToPoint(control);
	controls[1] = VectorToPoint(to);
	controls[2] = controls[1];

	((BShape *)user)->BezierTo(controls);
	return 0;
}


int
CubicToFunc(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *user)
{
	BPoint controls[3];

	controls[0] = VectorToPoint(control1);
	controls[1] = VectorToPoint(control2);
	controls[2] = VectorToPoint(to);

	((BShape *)user)->BezierTo(controls);
	return 0;
}


inline bool
is_white_space(uint32 charCode)
{
	switch (charCode) {
		case 0x0009:	/* tab */
		case 0x000b:	/* vertical tab */
		case 0x000c:	/* form feed */
		case 0x0020:	/* space */
		case 0x00a0:	/* non breaking space */
		case 0x000a:	/* line feed */
		case 0x000d:	/* carriage return */
		case 0x2028:	/* line separator */
		case 0x2029:	/* paragraph separator */
			return true;
	}

	return false;
}


//	#pragma mark -


/*! 
	\brief Constructor
	\param style Style object to which the ServerFont belongs
	\param size Character size in points
	\param rotation Rotation in degrees
	\param shear Shear (slant) in degrees. 45 <= shear <= 135
	\param flags Style flags as defined in <Font.h>
	\param spacing String spacing flag as defined in <Font.h>
*/
ServerFont::ServerFont(FontStyle& style, float size,
					   float rotation, float shear, float falseBoldWidth,
					   uint16 flags, uint8 spacing)
	: fStyle(&style),
	  fSize(size),
	  fRotation(rotation),
	  fShear(shear),
	  fFalseBoldWidth(falseBoldWidth),
	  fBounds(0, 0, 0, 0),
	  fFlags(flags),
	  fSpacing(spacing),
	  fDirection(style.Direction()),
	  fFace(style.Face()),
	  fEncoding(B_UNICODE_UTF8)
{
	fStyle->Acquire();
}


ServerFont::ServerFont()
	:
	fStyle(NULL)
{
	*this = *gFontManager->DefaultPlainFont();
}


/*! 
	\brief Copy Constructor
	\param font ServerFont to copy
*/
ServerFont::ServerFont(const ServerFont &font)
	:
	fStyle(NULL)
{
	*this = font;
}


/*! 
	\brief Removes itself as a dependency of its owning style.
*/
ServerFont::~ServerFont()
{
	fStyle->Release();
}


/*! 
	\brief Returns a copy of the specified font
	\param The font to copy from.
	\return A copy of the specified font
*/
ServerFont&
ServerFont::operator=(const ServerFont& font)
{
	if (font.fStyle) {
		fSize = font.fSize;
		fRotation = font.fRotation;
		fShear = font.fShear;
		fFalseBoldWidth = font.fFalseBoldWidth;
		fFlags = font.fFlags;
		fSpacing = font.fSpacing;
		fEncoding = font.fEncoding;
		fBounds = font.fBounds;
	
		SetStyle(font.fStyle);
	}

	return *this;
}


/*! 
	\brief Returns the number of strikes in the font
	\return The number of strikes in the font
*/
int32
ServerFont::CountTuned()
{
	return fStyle->TunedCount();
}


/*! 
	\brief Returns the file format of the font.
	\return Mostly B_TRUETYPE_WINDOWS :)
*/
font_file_format
ServerFont::FileFormat()
{
	return fStyle->FileFormat();
}


const char*
ServerFont::Style() const
{
	return fStyle->Name();
}


const char*
ServerFont::Family() const
{
	return fStyle->Family()->Name();
}


void
ServerFont::SetStyle(FontStyle* style)
{
	if (style && style != fStyle) {
		// detach from old style
		if (fStyle)
			fStyle->Release();

		// attach to new style
		fStyle = style;

		fStyle->Acquire();

		fFace = fStyle->Face();
		fDirection = fStyle->Direction();
	}
}


/*!
	\brief Sets the ServerFont instance to whatever font is specified
	This method will lock the font manager.

	\param familyID ID number of the family to set
	\param styleID ID number of the style to set
	\return B_OK if successful, B_ERROR if not
*/
status_t
ServerFont::SetFamilyAndStyle(uint16 familyID, uint16 styleID)
{
	FontStyle* style = NULL;

	if (gFontManager->Lock()) {
		style = gFontManager->GetStyle(familyID, styleID);
		if (style != NULL)
			style->Acquire();

		gFontManager->Unlock();
	}

	if (!style)
		return B_ERROR;

	SetStyle(style);
	style->Release();

	return B_OK;
}


/*!
	\brief Sets the ServerFont instance to whatever font is specified
	\param fontID the combination of family and style ID numbers
	\return B_OK if successful, B_ERROR if not
*/
status_t
ServerFont::SetFamilyAndStyle(uint32 fontID)
{
	uint16 style = fontID & 0xFFFF;
	uint16 family = (fontID & 0xFFFF0000) >> 16;
	
	return SetFamilyAndStyle(family, style);
}


void
ServerFont::SetFace(uint32 face)
{
	// TODO: change font style as requested!
	fFace = face;
}


/*!
	\brief Gets the ID values for the ServerFont instance in one shot
	\return the combination of family and style ID numbers
*/
uint32
ServerFont::GetFamilyAndStyle() const
{
	return (FamilyID() << 16) | StyleID();
}


FT_Face
ServerFont::GetTransformedFace(bool rotate, bool shear) const
{
	fStyle->Lock();
	FT_Face face = fStyle->FreeTypeFace();
	if (!face) {
		fStyle->Unlock();
		return NULL;
	}

	FT_Set_Char_Size(face, 0, int32(fSize * 64), 72, 72);

	if ((rotate && fRotation != 0) || (shear && fShear != 90)) {
		FT_Matrix rmatrix, smatrix;

		Angle rotationAngle(fRotation);
		rmatrix.xx = (FT_Fixed)( rotationAngle.Cosine() * 0x10000);
		rmatrix.xy = (FT_Fixed)(-rotationAngle.Sine() * 0x10000);
		rmatrix.yx = (FT_Fixed)( rotationAngle.Sine() * 0x10000);
		rmatrix.yy = (FT_Fixed)( rotationAngle.Cosine() * 0x10000);

		Angle shearAngle(fShear);
		smatrix.xx = (FT_Fixed)(0x10000); 
		smatrix.xy = (FT_Fixed)(-shearAngle.Cosine() * 0x10000);
		smatrix.yx = (FT_Fixed)(0);
		smatrix.yy = (FT_Fixed)(0x10000);

		// Multiply togheter and apply transform
		FT_Matrix_Multiply(&rmatrix, &smatrix);
		FT_Set_Transform(face, &smatrix, NULL);
	}

	// fStyle will be unlocked in PutTransformedFace()
	return face;
}


void
ServerFont::PutTransformedFace(FT_Face face) const
{
	// Reset transformation
	FT_Set_Transform(face, NULL, NULL);
	fStyle->Unlock();
}


status_t
ServerFont::GetGlyphShapes(const char charArray[], int32 numChars,
	BShape *shapeArray[]) const
{
	if (!charArray || numChars <= 0 || !shapeArray)
		return B_BAD_DATA;

	FT_Face face = GetTransformedFace(true, true);
	if (!face)
		return B_ERROR;

	FT_Outline_Funcs funcs;
	funcs.move_to = MoveToFunc;
	funcs.line_to = LineToFunc;
	funcs.conic_to = ConicToFunc;
	funcs.cubic_to = CubicToFunc;
	funcs.shift = 0;
	funcs.delta = 0;

	const char *string = charArray;
	for (int i = 0; i < numChars; i++) {
		shapeArray[i] = new BShape();
		FT_Load_Char(face, UTF8ToCharCode(&string), FT_LOAD_NO_BITMAP);
		FT_Outline outline = face->glyph->outline;
		FT_Outline_Decompose(&outline, &funcs, shapeArray[i]);
		shapeArray[i]->Close();
	}

	PutTransformedFace(face);
	return B_OK;
}


status_t
ServerFont::GetHasGlyphs(const char charArray[], int32 numChars,
	bool hasArray[]) const
{
	if (!charArray || numChars <= 0 || !hasArray)
		return B_BAD_DATA;

	FT_Face face = GetTransformedFace(false, false);
	if (!face)
		return B_ERROR;

	const char *string = charArray;
	for (int i = 0; i < numChars; i++)
		hasArray[i] = FT_Get_Char_Index(face, UTF8ToCharCode(&string)) > 0;

	PutTransformedFace(face);
	return B_OK;
}


status_t
ServerFont::GetEdges(const char charArray[], int32 numChars,
	edge_info edgeArray[]) const
{
	if (!charArray || numChars <= 0 || !edgeArray)
		return B_BAD_DATA;

	FT_Face face = GetTransformedFace(false, false);
	if (!face)
		return B_ERROR;

	const char *string = charArray;
	for (int i = 0; i < numChars; i++) {
		FT_Load_Char(face, UTF8ToCharCode(&string), FT_LOAD_NO_BITMAP);
		edgeArray[i].left = float(face->glyph->metrics.horiBearingX)
			/ 64 / fSize;
		edgeArray[i].right = float(face->glyph->metrics.horiBearingX 
			+ face->glyph->metrics.width - face->glyph->metrics.horiAdvance)
			/ 64 / fSize;
	}

	PutTransformedFace(face);
	return B_OK;
}


status_t
ServerFont::GetEscapements(const char charArray[], int32 numChars,
	escapement_delta delta, BPoint escapementArray[],
	BPoint offsetArray[]) const
{
	if (!charArray || numChars <= 0 || !escapementArray)
		return B_BAD_DATA;

	FT_Face face = GetTransformedFace(true, false);
	if (!face)
		return B_ERROR;

	const char *string = charArray;
	for (int i = 0; i < numChars; i++) {
		uint32 charCode = UTF8ToCharCode(&string);
		FT_Load_Char(face, charCode, FT_LOAD_NO_BITMAP);
		escapementArray[i].x = is_white_space(charCode) ? delta.space : delta.nonspace;
		escapementArray[i].x += float(face->glyph->advance.x) / 64;
		escapementArray[i].y = -float(face->glyph->advance.y) / 64;
		escapementArray[i].x /= fSize;
		escapementArray[i].y /= fSize;

		if (offsetArray) {
			// ToDo: According to the BeBook: "The offsetArray is applied by
			// the dynamic spacing in order to improve the relative position
			// of the character's width with relation to another character,
			// without altering the width." So this will probably depend on
			// the spacing mode.
			offsetArray[i].x = 0;
			offsetArray[i].y = 0;
		}
	}

	PutTransformedFace(face);
	return B_OK;
}


status_t
ServerFont::GetEscapements(const char charArray[], int32 numChars,
	escapement_delta delta, float widthArray[]) const
{
	if (!charArray || numChars <= 0 || !widthArray)
		return B_BAD_DATA;

	FT_Face face = GetTransformedFace(false, false);
	if (!face)
		return B_ERROR;

	const char *string = charArray;
	for (int i = 0; i < numChars; i++) {
		uint32 charCode = UTF8ToCharCode(&string);
		FT_Load_Char(face, charCode, FT_LOAD_NO_BITMAP);
		widthArray[i] = is_white_space(charCode) ? delta.space : delta.nonspace;
		widthArray[i] += float(face->glyph->metrics.horiAdvance) / 64.0;
		widthArray[i] /= fSize;
	}

	PutTransformedFace(face);
	return B_OK;
}


class BoundingBoxConsumer {
 public:
	BoundingBoxConsumer(Transformable& transform, BRect* rectArray,
			bool asString)
		: rectArray(rectArray)
		, stringBoundingBox(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN)
		, fAsString(asString)
		, fCurves(fPathAdaptor)
		, fContour(fCurves)
		, fTransformedOutline(fCurves, transform)
		, fTransformedContourOutline(fContour, transform)
		, fTransform(transform)
	{
	}
	void Start() {}
	void Finish(double x, double y) {}
	void ConsumeEmptyGlyph(int32 index, uint32 charCode, double x, double y) {}
	bool ConsumeGlyph(int32 index, uint32 charCode, const GlyphCache* glyph,
		FontCacheEntry* entry, double x, double y)
	{
		if (glyph->data_type != glyph_data_outline) {
			const agg::rect_i& r = glyph->bounds;
			if (fAsString) {
				rectArray[index].left = r.x1 + x;
				rectArray[index].top = r.y1 + y - 1;
				rectArray[index].right = r.x2 + x + 1;
				rectArray[index].bottom = r.y2 + y + 1;
			} else {
				if (rectArray) {
					rectArray[index].left = r.x1;
					rectArray[index].top = r.y1 - 1;
					rectArray[index].right = r.x2 + 1;
					rectArray[index].bottom = r.y2 + 1;
				} else {
					stringBoundingBox = stringBoundingBox
						| BRect(r.x1 + x, r.y1 + y - 1,
							r.x2 + x + 1, r.y2 + y + 1);
				}
			}
		} else {
			if (fAsString) {
				entry->InitAdaptors(glyph, x, y,
						fMonoAdaptor, fGray8Adaptor, fPathAdaptor);
			} else {
				entry->InitAdaptors(glyph, 0, 0,
						fMonoAdaptor, fGray8Adaptor, fPathAdaptor);
			}
			double left = 0.0;
			double top = 0.0;
			double right = -1.0;
			double bottom = -1.0;
			uint32 pathID[1];
			pathID[0] = 0;
			// TODO: use fContour if falseboldwidth is > 0
			agg::bounding_rect(fTransformedOutline, pathID, 0, 1,
				&left, &top, &right, &bottom);

			if (rectArray) {
				rectArray[index] = BRect(left, top, right, bottom);
			} else {
				stringBoundingBox = stringBoundingBox
					| BRect(left, top, right, bottom);
			}
		}
		return true;
	}

	BRect*								rectArray;
	BRect								stringBoundingBox;

 private:
	bool								fAsString;
	FontCacheEntry::GlyphPathAdapter	fPathAdaptor;
	FontCacheEntry::GlyphGray8Adapter	fGray8Adaptor;
	FontCacheEntry::GlyphMonoAdapter	fMonoAdaptor;

	FontCacheEntry::CurveConverter		fCurves;
	FontCacheEntry::ContourConverter	fContour;

	FontCacheEntry::TransformedOutline	fTransformedOutline;
	FontCacheEntry::TransformedContourOutline fTransformedContourOutline;

	Transformable&						fTransform;
};


status_t
ServerFont::GetBoundingBoxes(const char* string, int32 numChars,
	BRect rectArray[], bool stringEscapement, font_metric_mode mode,
	escapement_delta delta, bool asString)
{
	// TODO: The font_metric_mode is not used
	if (!string || numChars <= 0 || !rectArray)
		return B_BAD_DATA;

	bool kerning = true; // TODO make this a property?

	Transformable transform(EmbeddedTransformation());

	BoundingBoxConsumer consumer(transform, rectArray, asString);
	if (GlyphLayoutEngine::LayoutGlyphs(consumer, *this, string, numChars,
		stringEscapement ? &delta : NULL, kerning, fSpacing))
		return B_OK;
	return B_ERROR;
}


status_t
ServerFont::GetBoundingBoxesForStrings(char *charArray[], int32 lengthArray[], 
	int32 numStrings, BRect rectArray[], font_metric_mode mode, escapement_delta deltaArray[])
{
	// TODO: The font_metric_mode is never used
	if (!charArray || !lengthArray|| numStrings <= 0 || !rectArray || !deltaArray)
		return B_BAD_DATA;

	bool kerning = true; // TODO make this a property?

	Transformable transform(EmbeddedTransformation());

	for (int32 i = 0; i < numStrings; i++) {
		int32 numChars = lengthArray[i];
		const char* string = charArray[i];
		escapement_delta delta = deltaArray[i];

		BoundingBoxConsumer consumer(transform, NULL, true);
		if (!GlyphLayoutEngine::LayoutGlyphs(consumer, *this, string, numChars,
			&delta, kerning, fSpacing))
			return B_ERROR;

		rectArray[i] = consumer.stringBoundingBox;
	}

	return B_OK;
}


class StringWidthConsumer {
 public:
	StringWidthConsumer() : width(0.0) {}
	void Start() {}
	void Finish(double x, double y) { width = x; }
	void ConsumeEmptyGlyph(int32 index, uint32 charCode, double x, double y) {}
	bool ConsumeGlyph(int32 index, uint32 charCode, const GlyphCache* glyph,
		FontCacheEntry* entry, double x, double y)
	{ return true; }

	float width;
};


float
ServerFont::StringWidth(const char *string, int32 numChars,
	const escapement_delta* deltaArray) const
{
	if (!string || numChars <= 0)
		return 0.0;

	bool kerning = true; // TODO make this a property?

	StringWidthConsumer consumer;
	if (!GlyphLayoutEngine::LayoutGlyphs(consumer, *this, string, numChars,
		deltaArray, kerning, fSpacing))
		return 0.0;

	return consumer.width;
}


/*! 
	\brief Returns a BRect which encloses the entire font
	\return A BRect which encloses the entire font
*/
BRect
ServerFont::BoundingBox()
{
	// TODO: fBounds is nowhere calculated!
	return fBounds;
}


/*! 
	\brief Obtains the height values for characters in the font in its current state
	\param fh pointer to a font_height object to receive the values for the font
*/
void
ServerFont::GetHeight(font_height& height) const
{
	fStyle->GetHeight(fSize, height);
}


void
ServerFont::TruncateString(BString* inOut, uint32 mode, float width) const
{
	if (!inOut)
		return;

	// the width of the "…" glyph
	float ellipsisWidth = StringWidth(B_UTF8_ELLIPSIS, 1);
	const char *string = inOut->String();
	int32 length = inOut->Length();

	// temporary array to hold result
	char *result = new char[length + 3];

	// count the individual glyphs
	int32 numChars = UTF8CountChars(string, -1);

	// get the escapement of each glyph in font units
	float *escapementArray = new float[numChars];
	static escapement_delta delta = (escapement_delta){ 0.0, 0.0 };
	if (GetEscapements(string, numChars, delta, escapementArray) == B_OK) {
		truncate_string(string, mode, width, result, escapementArray, fSize,
			ellipsisWidth, length, numChars);

		inOut->SetTo(result);
	}

	delete[] escapementArray;
	delete[] result;
}


Transformable
ServerFont::EmbeddedTransformation() const
{
	// TODO: cache this?
	Transformable transform;

	transform.ShearBy(B_ORIGIN, (90.0 - fShear) * PI / 180.0, 0.0);
	transform.RotateBy(B_ORIGIN, -fRotation * PI / 180.0);

	return transform;
}

