// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef RFB_HIGHRESOLUTIONWHEEL_H
#define RFB_HIGHRESOLUTIONWHEEL_H

// Include the application's rfb.h (protocol types and byte swapping) first.
static_assert(sizeof(rfbHighResolutionWheelMsg) == sz_rfbHighResolutionWheelMsg,
	"High-resolution wheel wire layout must be exactly eight bytes");

inline rfbHighResolutionWheelMsg MakeHighResolutionWheel(unsigned axis, int delta,
	unsigned x, unsigned y)
{
	rfbHighResolutionWheelMsg msg;
	msg.type = rfbHighResolutionWheel;
	msg.axis = (CARD8)axis;
	msg.delta = Swap16IfLE((CARD16)delta);
	msg.x = Swap16IfLE(x);
	msg.y = Swap16IfLE(y);
	return msg;
}

inline bool DecodeHighResolutionWheel(const rfbHighResolutionWheelMsg& msg,
	unsigned width, unsigned height, unsigned& axis, int& delta,
	unsigned& x, unsigned& y)
{
	axis = msg.axis;
	unsigned bits = Swap16IfLE(msg.delta);
	delta = bits < 0x8000 ? (int)bits : (int)bits - 0x10000;
	x = Swap16IfLE(msg.x);
	y = Swap16IfLE(msg.y);
	return msg.type == rfbHighResolutionWheel && axis <= rfbWheelHorizontal &&
		x < width && y < height;
}
#endif
