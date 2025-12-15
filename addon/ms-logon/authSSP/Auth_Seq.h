// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2004 Martin Scharpf. All Rights Reserved.
//


typedef struct _AUTH_SEQ {
   BOOL fInitialized;
   BOOL fHaveCredHandle;
   BOOL fHaveCtxtHandle;
   CredHandle hcred;
   struct _SecHandle hctxt;
} AUTH_SEQ, *PAUTH_SEQ;

typedef struct _Fn {
ACCEPT_SECURITY_CONTEXT_FN       _AcceptSecurityContext;
ACQUIRE_CREDENTIALS_HANDLE_FN    _AcquireCredentialsHandle;
COMPLETE_AUTH_TOKEN_FN           _CompleteAuthToken;
INITIALIZE_SECURITY_CONTEXT_FN   _InitializeSecurityContext;
DELETE_SECURITY_CONTEXT_FN       _DeleteSecurityContext;
FREE_CONTEXT_BUFFER_FN           _FreeContextBuffer;
FREE_CREDENTIALS_HANDLE_FN       _FreeCredentialsHandle;
QUERY_SECURITY_PACKAGE_INFO_FN   _QuerySecurityPackageInfo;
IMPERSONATE_SECURITY_CONTEXT_FN  _ImpersonateSecurityContext;
REVERT_SECURITY_CONTEXT_FN       _RevertSecurityContext;
} Fn;

