// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//

#pragma once

// Must be defined before security.h
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <wincrypt.h>
#include <security.h>
#include <schannel.h>
#include <ncrypt.h>
#include <vector>

// Forward declaration
class VSocket;

//
// SchannelTLS — server-side TLS wrapper using Windows SChannel/SSPI.
//
// Usage per connection:
//   1. SchannelTLS tls;
//   2. tls.LoadCertByThumbprint(thumbprint) or tls.EnsureSelfSignedCert()
//   3. tls.ServerHandshake(socket)   — blocks until TLS handshake done
//   4. Use tls.Send() / tls.Recv() instead of raw socket I/O
//   5. tls.Shutdown(socket)
//
class SchannelTLS
{
public:
    SchannelTLS();
    ~SchannelTLS();

    // Load an existing certificate from the Windows cert store by thumbprint.
    // thumbprint: hex string, e.g. "a1b2c3..." (40 chars, no spaces).
    bool LoadCertByThumbprint(const char* thumbprint);

    // Find or create a self-signed certificate stored under CN=UltraVNC-WebViewer
    // in the local machine MY store. Saves the thumbprint to outThumbprint (41 chars).
    bool EnsureSelfSignedCert(char* outThumbprint);

    // Perform the server-side TLS handshake on an already-accepted socket.
    // Returns true on success, false on failure (socket should be closed).
    bool ServerHandshake(VSocket* sock);

    // Send encrypted data. Returns false on error.
    bool Send(VSocket* sock, const char* buf, int len);

    // Receive decrypted data. Fills buf up to len bytes, sets *received.
    // Returns false on fatal error (connection closed counts as false with *received==0).
    bool Recv(VSocket* sock, char* buf, int len, int* received);

    // Clean TLS shutdown (sends close_notify).
    void Shutdown(VSocket* sock);

    // True after a successful ServerHandshake().
    bool IsReady() const { return m_ready; }

private:
    // Raw socket send/recv helpers (bypass VSocket HTTP wrappers)
    bool RawSend(VSocket* sock, const BYTE* data, DWORD len);
    bool RawRecv(VSocket* sock, BYTE* data, DWORD len, DWORD* received);

    // Token exchange during handshake
    bool HandshakeLoop(VSocket* sock, bool firstCall, SecBuffer* pExtraData);

    PCCERT_CONTEXT          m_certCtx;
    CredHandle              m_credHandle;
    bool                    m_credValid;
    CtxtHandle              m_ctxHandle;
    bool                    m_ctxValid;
    bool                    m_ready;

    SecPkgContext_StreamSizes m_streamSizes;

    // Undecrypted bytes waiting to be processed
    std::vector<BYTE>       m_rawBuf;
    // Decrypted plaintext buffered for Recv()
    std::vector<BYTE>       m_plainBuf;
};
