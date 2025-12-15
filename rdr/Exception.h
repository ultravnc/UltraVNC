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


#ifndef __RDR_EXCEPTION_H__
#define __RDR_EXCEPTION_H__

#include <stdio.h>
#include <string.h>

namespace rdr {

  struct Exception {
    enum { len = 256 };
    char str_[len];
    Exception(const char* s=0, const char* e="rdr::Exception") {
      str_[0] = 0;
      strncat_s(str_, e, len-1);
      if (s) {
        strncat_s(str_, ": ", len-1-strlen(str_));
        strncat_s(str_, s, len-1-strlen(str_));
      }
    }
    virtual const char* str() const { return str_; }
  };

  struct SystemException : public Exception {
    int err;
    SystemException(const char* s, int err_) : err(err_) {
      str_[0] = 0;
      strncat_s(str_, "rdr::SystemException: ", len-1);
      strncat_s(str_, s, len-1-strlen(str_));
      strncat_s(str_, ": ", len-1-strlen(str_));
	  char errorbuffer[1024];
	  strerror_s(errorbuffer, 1024, err);
      strncat_s(str_, errorbuffer, len-1-strlen(str_));
      strncat_s(str_, " (", len-1-strlen(str_));
      char buf[20];
      sprintf_s(buf,"%d",err);
      strncat_s(str_, buf, len-1-strlen(str_));
      strncat_s(str_, ")", len-1-strlen(str_));
    }
  }; 

  struct TimedOut : public Exception {
    TimedOut(const char* s=0) : Exception(s,"rdr::TimedOut") {}
  };
 
  struct EndOfStream : public Exception {
    EndOfStream(const char* s=0) : Exception(s,"rdr::EndOfStream") {}
  };

  struct FrameException : public Exception {
    FrameException(const char* s=0) : Exception(s,"rdr::FrameException") {}
  };

}

#endif
