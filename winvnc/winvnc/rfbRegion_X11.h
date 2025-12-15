// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002-2003 RealVNC Ltd. All Rights Reserved.
//


// Cross-platform Region class based on the X11 region implementation

#ifndef __RFB_REGION_X11_INCLUDED__
#define __RFB_REGION_X11_INCLUDED__

#include "rfbRect.h"
#include <Xregion/Xregion.h>
#include <vector>

namespace rfb {

  // rfb::Region
  // See Region.h for description of interface.

  class Region {
  public:
    // Create an empty region
    Region();
    // Create a rectangular region
    Region(int x1, int y1, int x2, int y2);
    Region(const Rect& r);

    Region(const Region& r);
    Region &operator=(const Region& src);

    ~Region();

    // the following methods alter the region in place:

    void clear();
    void reset(const Rect& r);
    void translate(const rfb::Point& delta);
    void setOrderedRects(const std::vector<Rect>& rects);

    void assign_intersect(const Region& r);
    void assign_union(const Region& r);
    void assign_subtract(const Region& r);

    // the following three operations return a new region:

    Region intersect(const Region& r) const;
    Region union_(const Region& r) const;
    Region subtract(const Region& r) const;

    bool equals(const Region& b) const;
    bool is_empty() const;

    bool get_rects(std::vector<Rect>& rects, bool left2right=true,
                   bool topdown=true) const;
    Rect get_bounding_rect() const;
	int Numrects();

    void debug_print(const char *prefix) const;

  protected:
    Region(struct _XRegion* rgn);
    struct _XRegion* replaceXrgn(struct _XRegion* newrgn);

    struct _XRegion*  Xrgn;
  };

  typedef Region Region2D;

}

#endif // __RFB_REGION_X11_INCLUDED__
