/*
 * Copyright (c) 2004, Marcus Overhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <string.h>
#include "matroska_util.h"


bigtime_t
get_duration_in_us(uint64 duration)
{
	if (duration > 0)
		return duration / 1000 + ((duration % 1000) ? 1 : 0);
	else
		return 12 * 60 * 60 * 1000000LL;
}


float
get_frame_rate(uint64 default_duration)
{
	if (default_duration > 1)
		return 1000000000.0 / default_duration;
	else
		return 29.97;
}


uint64
get_frame_count_by_frame_rate(uint64 duration, float frame_rate)
{
	return uint64((double(duration) * double(frame_rate)) / 1000000000.0);
}


uint64
get_frame_count_by_default_duration(uint64 duration, uint64 default_duration)
{
	if (duration > 0 && default_duration > 1)
		return (duration / default_duration) + ((duration % default_duration) ? 1 : 0);
	if (duration > 0)
		return (duration * 2997) / 100000000000ull;
	return 12 * 60 * 60 * 2997 / 100;
}
