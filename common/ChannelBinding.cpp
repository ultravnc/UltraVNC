// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
//
#include "ChannelBinding.h"

namespace ChannelBinding
{
    // Map a signatureAlgorithm OID to the CryptHashCertificate ALG_ID required
    // by RFC 5929 section 4.1. Note: for composite signature OIDs like
    // sha256RSA, CryptFindOIDInfo may return a *signature* ALG_ID
    // (CALG_RSA_SIGN) which is unusable for hashing, so we check the OID
    // display name and the ALG_ID for a real digest algorithm.
    static ALG_ID HashAlgForCertificate(PCCERT_CONTEXT cert)
    {
        const char* oid = cert->pCertInfo->SignatureAlgorithm.pszObjId;
        PCCRYPT_OID_INFO info = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY, (void*)oid, 0);
        if (info)
        {
            switch (info->Algid)
            {
            case CALG_SHA_512: return CALG_SHA_512;
            case CALG_SHA_384: return CALG_SHA_384;
            case CALG_SHA_256: return CALG_SHA_256;
            default: break; // MD5/SHA-1/signature alg -> handled below
            }
            // Composite signature OIDs (sha256RSA, ecdsaSHA256, ...) often
            // carry the hash name in the display name.
            if (info->pwszName)
            {
                if (wcsstr(info->pwszName, L"512")) return CALG_SHA_512;
                if (wcsstr(info->pwszName, L"384")) return CALG_SHA_384;
                if (wcsstr(info->pwszName, L"256")) return CALG_SHA_256;
            }
        }
        // MD5/SHA-1 signatures, or unrecognized -> SHA-256 per RFC 5929.
        return CALG_SHA_256;
    }

    bool ServerEndPointHash(PCCERT_CONTEXT cert, std::vector<BYTE>& outHash)
    {
        if (!cert || !cert->pbCertEncoded || cert->cbCertEncoded == 0)
            return false;
        BYTE buf[64]; // SHA-512 is the largest supported digest
        DWORD len = sizeof(buf);
        if (!CryptHashCertificate(0, HashAlgForCertificate(cert), 0,
                                  cert->pbCertEncoded, cert->cbCertEncoded,
                                  buf, &len))
            return false;
        outHash.assign(buf, buf + len);
        return true;
    }
}
