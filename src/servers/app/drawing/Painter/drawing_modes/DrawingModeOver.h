/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * DrawingMode implementing B_OP_OVER on B_RGBA32.
 *
 */

#ifndef DRAWING_MODE_OVER_H
#define DRAWING_MODE_OVER_H

#include "DrawingMode.h"

// BLEND_OVER
#define BLEND_OVER(d, s1, s2, s3, a) \
{ \
	pixel32 _p; \
	_p.data32 = *(uint32*)d; \
	d[0] = 	(((((s3) - _p.data8[0]) * (a)) + (_p.data8[0] << 8)) >> 8); \
	d[1] = 	(((((s2) - _p.data8[1]) * (a)) + (_p.data8[1] << 8)) >> 8); \
	d[2] = 	(((((s1) - _p.data8[2]) * (a)) + (_p.data8[2] << 8)) >> 8); \
	d[3] = 255; \
}

// ASSIGN_OVER
#define ASSIGN_OVER(d1, d2, d3, da, s1, s2, s3) \
{ \
	(d1) = (s1); \
	(d2) = (s2); \
	(d3) = (s3); \
	(da) = 255; \
}


template<class Order>
class DrawingModeOver : public DrawingMode {
 public:
	// constructor
	DrawingModeOver()
		: DrawingMode()
	{
	}

	// blend_pixel
	virtual	void blend_pixel(int x, int y, const color_type& c, uint8 cover)
	{
		if (fPatternHandler->IsHighColor(x, y)) {
			uint8* p = fBuffer->row(y) + (x << 2);
			rgb_color color = fPatternHandler->HighColor().GetColor32();
			if (cover == 255) {
				ASSIGN_OVER(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
							color.red, color.green, color.blue);
			} else {
				BLEND_OVER(p, color.red, color.green, color.blue, cover);
			}
		}
	}

	// blend_hline
	virtual	void blend_hline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
		if(cover == 255) {
			rgb_color color = fPatternHandler->HighColor().GetColor32();
			uint32 v;
			uint8* p8 = (uint8*)&v;
			p8[Order::R] = (uint8)color.red;
			p8[Order::G] = (uint8)color.green;
			p8[Order::B] = (uint8)color.blue;
			p8[Order::A] = 255;
			uint32* p32 = (uint32*)(fBuffer->row(y)) + x;
			do {
				if (fPatternHandler->IsHighColor(x, y))
					*p32 = v;
				p32++;
				x++;
			} while(--len);
		} else {
			uint8* p = fBuffer->row(y) + (x << 2);
			rgb_color color = fPatternHandler->HighColor().GetColor32();
			do {
				if (fPatternHandler->IsHighColor(x, y)) {
					BLEND_OVER(p, color.red, color.green, color.blue, cover);
				}
				x++;
				p += 4;
			} while(--len);
		}
	}

	// blend_vline
	virtual	void blend_vline(int x, int y, unsigned len, 
							 const color_type& c, uint8 cover)
	{
printf("DrawingModeOver::blend_vline()\n");
	}

	// blend_solid_hspan
	virtual	void blend_solid_hspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color color = fPatternHandler->HighColor().GetColor32();
		do {
			if (fPatternHandler->IsHighColor(x, y)) {
				if (*covers) {
					if (*covers == 255) {
						ASSIGN_OVER(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									color.red, color.green, color.blue);
					} else {
						BLEND_OVER(p, color.red, color.green, color.blue, *covers);
					}
				}
			}
			covers++;
			p += 4;
			x++;
		} while(--len);
	}



	// blend_solid_vspan
	virtual	void blend_solid_vspan(int x, int y, unsigned len, 
								   const color_type& c, const uint8* covers)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		rgb_color color = fPatternHandler->HighColor().GetColor32();
		do {
			if (fPatternHandler->IsHighColor(x, y)) {
				if (*covers) {
					if (*covers == 255) {
						ASSIGN_OVER(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									color.red, color.green, color.blue);
					} else {
						BLEND_OVER(p, color.red, color.green, color.blue, *covers);
					}
				}
			}
			covers++;
			p += fBuffer->stride();
			y++;
		} while(--len);
	}


	// blend_color_hspan
	virtual	void blend_color_hspan(int x, int y, unsigned len, 
								   const color_type* colors, 
								   const uint8* covers,
								   uint8 cover)
	{
		uint8* p = fBuffer->row(y) + (x << 2);
		if (covers) {
			// non-solid opacity
			do {
//				if (*covers) {
if (*covers && (colors->a & 0xff)) {
					if (*covers == 255) {
						ASSIGN_OVER(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
									colors->r, colors->g, colors->b);
					} else {
						BLEND_OVER(p, colors->r, colors->g, colors->b, *covers);
					}
				}
				covers++;
				p += 4;
				++colors;
			} while(--len);
		} else {
			// solid full opcacity
			if (cover == 255) {
				do {
if (colors->a & 0xff) {
					ASSIGN_OVER(p[Order::R], p[Order::G], p[Order::B], p[Order::A],
								colors->r, colors->g, colors->b);
}
					p += 4;
					++colors;
				} while(--len);
			// solid partial opacity
			} else if (cover) {
				do {
					BLEND_OVER(p, colors->r, colors->g, colors->b, cover);
					p += 4;
					++colors;
				} while(--len);
			}
		}
	}


	// blend_color_vspan
	virtual	void blend_color_vspan(int x, int y, unsigned len, 
								   const color_type* colors, 
								   const uint8* covers,
								   uint8 cover)
	{
printf("DrawingModeOver::blend_color_vspan()\n");
	}

};

typedef DrawingModeOver<agg::order_rgba32> DrawingModeRGBA32Over;
typedef DrawingModeOver<agg::order_argb32> DrawingModeARGB32Over;
typedef DrawingModeOver<agg::order_abgr32> DrawingModeABGR32Over;
typedef DrawingModeOver<agg::order_bgra32> DrawingModeBGRA32Over;

#endif // DRAWING_MODE_OVER_H

