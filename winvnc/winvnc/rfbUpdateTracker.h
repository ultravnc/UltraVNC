// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//


#ifndef __RFB_UPDATETRACKER_INCLUDED__
#define __RFB_UPDATETRACKER_INCLUDED__

#include "rfbRect.h"
#include "rfbRegion.h"

namespace rfb {

	struct UpdateInfo {
		RectVector cached;
		RectVector copied;
		Point copy_delta;
		RectVector changed;
	};

	class UpdateTracker {
	public:
		UpdateTracker() {};
		virtual ~UpdateTracker() {};

		virtual void add_changed(const Region2D &region) = 0;
		virtual void add_cached(const Region2D &region) = 0;
		virtual void add_copied(const Region2D &dest, const Point &delta) = 0;

		
	};

	class ClippedUpdateTracker : public UpdateTracker {
	public:
		ClippedUpdateTracker(UpdateTracker &child_) : child(child_) {};
		ClippedUpdateTracker(UpdateTracker &child_,
			const Region2D &cliprgn_) : child(child_), cliprgn(cliprgn_) {};
		virtual ~ClippedUpdateTracker() {};

		virtual void set_clip_region(const Region2D cliprgn_) {cliprgn = cliprgn_;};

		virtual void add_changed(const Region2D &region);
		virtual void add_cached(const Region2D &region);
		virtual void add_copied(const Region2D &dest, const Point &delta);

	protected:
		UpdateTracker &child;
		Region2D cliprgn;
	};

	class SimpleUpdateTracker : public UpdateTracker {
	public:
		SimpleUpdateTracker(bool use_copyrect=false);
		virtual ~SimpleUpdateTracker();

		virtual void enable_copyrect(bool enable) {copy_enabled=enable;};

		virtual void add_changed(const Region2D &region);
		virtual void add_cached(const Region2D &region);
		virtual void add_copied(const Region2D &dest, const Point &delta);

		// Fill the supplied UpdateInfo structure with update information
		// Also removes the updates that are returned from the update tracker
		virtual void flush_update(UpdateInfo &info, const Region2D &cliprgn);
		virtual void flush_update(UpdateTracker &info, const Region2D &cliprgn);

		// Pass the current updates to the supplied tracker
		// Does not affect internal state of this tracker
		virtual void get_update(UpdateInfo &to) const;
		virtual void get_update(UpdateTracker &to) const;

		// Get the changed/copied regions
		virtual const Region2D& get_changed_region() const {return changed;};
		virtual const Region2D& get_cached_region() const {return cached;};
		virtual const Region2D& get_copied_region() const {return copied;};


		virtual bool is_empty() const {return changed.is_empty() && copied.is_empty() && cached.is_empty();};

		virtual void clear() {
			changed.clear();
			copied.clear();
			cached.clear();
		};
	protected:
		Region2D changed;
		Region2D cached;
		Region2D copied;
		Point copy_delta;
		bool copy_enabled;
	};

};

#endif /* __RFB_UPDATETRACKER_INCLUDED__ */
