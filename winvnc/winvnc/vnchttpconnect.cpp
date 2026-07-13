// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2000 Const Kaplinsky. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


// vncHTTPConnect.cpp

// Implementation of the HTTP server class

#include "stdhdrs.h"
#include <stdint.h>
#include <process.h>
#include "vsocket.h"
#include "vnchttpconnect.h"
#include "vncserver.h"
#include <omnithread.h>
#include "resource.h"
#include <ctype.h>
#include "SettingsManager.h"
#include "SchannelTLS.h"
#include <vector>

//	[v1.0.2-jp1 fix]
extern	HINSTANCE	hInstResDLL;
extern bool			fShutdownOrdered;

// HTTP messages / message formats
const char HTTP_MSG_OK[] = "HTTP/1.0 200 OK\r\n\r\n";

const char HTTP_MSG_NOSOCKCONN [] =
"<HTML>\n"
"  <HEAD><TITLE>UltraVNC desktop</TITLE></HEAD>\n"
"  <BODY>\n"
"    <H1>Connections Disabled</H1>\n"
"    The requested desktop is not configured to accept incoming connections.\n"
"  </BODY>\n"
"</HTML>\n";

const char HTTP_MSG_NOSUCHFILE [] =
"HTTP/1.0 404 Not Found\r\n\r\n"
"<HTML>\n"
"  <HEAD><TITLE>404 Not Found</TITLE></HEAD>\n"
"  <BODY>\n"
"    <H1>Not Found</H1>\n"
"    The requested file could not be found.\n"
"  </BODY>\n"
"</HTML>\n";



// Filename to resource ID mappings for the WebViewer files:
typedef struct _FileToResourceMap {
	char *filename;
	char *type;
	int resourceID;
} FileMap;

const FileMap filemapping [] = {
	{"/core/base64.js",	"JS",	IDR_WEBVIEWER_CORE_BASE64_JS},
	{"/core/crypto/aes.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_AES_JS},
	{"/core/crypto/bigint.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_BIGINT_JS},
	{"/core/crypto/crypto.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_CRYPTO_JS},
	{"/core/crypto/des.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_DES_JS},
	{"/core/crypto/dh.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_DH_JS},
	{"/core/crypto/md5.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_MD5_JS},
	{"/core/crypto/rsa.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_RSA_JS},
	{"/core/decoders/copyrect.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_COPYRECT_JS},
	{"/core/decoders/h264.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_H264_JS},
	{"/core/decoders/hextile.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_HEXTILE_JS},
	{"/core/decoders/jpeg.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_JPEG_JS},
	{"/core/decoders/raw.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_RAW_JS},
	{"/core/decoders/rre.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_RRE_JS},
	{"/core/decoders/tight.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_TIGHT_JS},
	{"/core/decoders/tightpng.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_TIGHTPNG_JS},
	{"/core/decoders/zlib.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_ZLIB_JS},
	{"/core/decoders/zrle.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_ZRLE_JS},
	{"/core/deflator.js",	"JS",	IDR_WEBVIEWER_CORE_DEFLATOR_JS},
	{"/core/display.js",	"JS",	IDR_WEBVIEWER_CORE_DISPLAY_JS},
	{"/core/encodings.js",	"JS",	IDR_WEBVIEWER_CORE_ENCODINGS_JS},
	{"/core/inflator.js",	"JS",	IDR_WEBVIEWER_CORE_INFLATOR_JS},
	{"/core/input/domkeytable.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_DOMKEYTABLE_JS},
	{"/core/input/fixedkeys.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_FIXEDKEYS_JS},
	{"/core/input/gesturehandler.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_GESTUREHANDLER_JS},
	{"/core/input/keyboard.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_KEYBOARD_JS},
	{"/core/input/keysym.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_KEYSYM_JS},
	{"/core/input/keysymdef.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_KEYSYMDEF_JS},
	{"/core/input/util.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_UTIL_JS},
	{"/core/input/vkeys.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_VKEYS_JS},
	{"/core/input/xtscancodes.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_XTSCANCODES_JS},
	{"/core/ra2.js",	"JS",	IDR_WEBVIEWER_CORE_RA2_JS},
	{"/core/rfb.js",	"JS",	IDR_WEBVIEWER_CORE_RFB_JS},
	{"/core/util/browser.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_BROWSER_JS},
	{"/core/util/cursor.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_CURSOR_JS},
	{"/core/util/element.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_ELEMENT_JS},
	{"/core/util/events.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_EVENTS_JS},
	{"/core/util/eventtarget.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_EVENTTARGET_JS},
	{"/core/util/int.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_INT_JS},
	{"/core/util/logging.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_LOGGING_JS},
	{"/core/util/strings.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_STRINGS_JS},
	{"/core/websock.js",	"JS",	IDR_WEBVIEWER_CORE_WEBSOCK_JS},
	{"/index.html",	"HTML",	IDR_WEBVIEWER_INDEX_HTML},
	{"/",	"HTML",	IDR_WEBVIEWER_INDEX_HTML},
	{"/webviewer/",	"HTML",	IDR_WEBVIEWER_INDEX_HTML},
	{"/webviewer/index.html",	"HTML",	IDR_WEBVIEWER_INDEX_HTML},
	{"/vendor/pako/lib/utils/common.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_UTILS_COMMON_JS},
	{"/vendor/pako/lib/zlib/adler32.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_ADLER32_JS},
	{"/vendor/pako/lib/zlib/constants.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_CONSTANTS_JS},
	{"/vendor/pako/lib/zlib/crc32.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_CRC32_JS},
	{"/vendor/pako/lib/zlib/deflate.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_DEFLATE_JS},
	{"/vendor/pako/lib/zlib/gzheader.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_GZHEADER_JS},
	{"/vendor/pako/lib/zlib/inffast.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_INFFAST_JS},
	{"/vendor/pako/lib/zlib/inflate.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_INFLATE_JS},
	{"/vendor/pako/lib/zlib/inftrees.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_INFTREES_JS},
	{"/vendor/pako/lib/zlib/messages.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_MESSAGES_JS},
	{"/vendor/pako/lib/zlib/trees.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_TREES_JS},
	{"/vendor/pako/lib/zlib/zstream.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_ZSTREAM_JS},
};






const FileMap filemapping2 [] = {
	{"/core/base64.js",	"JS",	IDR_WEBVIEWER_CORE_BASE64_JS},
	{"/core/crypto/aes.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_AES_JS},
	{"/core/crypto/bigint.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_BIGINT_JS},
	{"/core/crypto/crypto.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_CRYPTO_JS},
	{"/core/crypto/des.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_DES_JS},
	{"/core/crypto/dh.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_DH_JS},
	{"/core/crypto/md5.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_MD5_JS},
	{"/core/crypto/rsa.js",	"JS",	IDR_WEBVIEWER_CORE_CRYPTO_RSA_JS},
	{"/core/decoders/copyrect.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_COPYRECT_JS},
	{"/core/decoders/h264.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_H264_JS},
	{"/core/decoders/hextile.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_HEXTILE_JS},
	{"/core/decoders/jpeg.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_JPEG_JS},
	{"/core/decoders/raw.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_RAW_JS},
	{"/core/decoders/rre.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_RRE_JS},
	{"/core/decoders/tight.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_TIGHT_JS},
	{"/core/decoders/tightpng.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_TIGHTPNG_JS},
	{"/core/decoders/zlib.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_ZLIB_JS},
	{"/core/decoders/zrle.js",	"JS",	IDR_WEBVIEWER_CORE_DECODERS_ZRLE_JS},
	{"/core/deflator.js",	"JS",	IDR_WEBVIEWER_CORE_DEFLATOR_JS},
	{"/core/display.js",	"JS",	IDR_WEBVIEWER_CORE_DISPLAY_JS},
	{"/core/encodings.js",	"JS",	IDR_WEBVIEWER_CORE_ENCODINGS_JS},
	{"/core/inflator.js",	"JS",	IDR_WEBVIEWER_CORE_INFLATOR_JS},
	{"/core/input/domkeytable.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_DOMKEYTABLE_JS},
	{"/core/input/fixedkeys.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_FIXEDKEYS_JS},
	{"/core/input/gesturehandler.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_GESTUREHANDLER_JS},
	{"/core/input/keyboard.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_KEYBOARD_JS},
	{"/core/input/keysym.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_KEYSYM_JS},
	{"/core/input/keysymdef.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_KEYSYMDEF_JS},
	{"/core/input/util.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_UTIL_JS},
	{"/core/input/vkeys.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_VKEYS_JS},
	{"/core/input/xtscancodes.js",	"JS",	IDR_WEBVIEWER_CORE_INPUT_XTSCANCODES_JS},
	{"/core/ra2.js",	"JS",	IDR_WEBVIEWER_CORE_RA2_JS},
	{"/core/rfb.js",	"JS",	IDR_WEBVIEWER_CORE_RFB_JS},
	{"/core/util/browser.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_BROWSER_JS},
	{"/core/util/cursor.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_CURSOR_JS},
	{"/core/util/element.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_ELEMENT_JS},
	{"/core/util/events.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_EVENTS_JS},
	{"/core/util/eventtarget.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_EVENTTARGET_JS},
	{"/core/util/int.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_INT_JS},
	{"/core/util/logging.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_LOGGING_JS},
	{"/core/util/strings.js",	"JS",	IDR_WEBVIEWER_CORE_UTIL_STRINGS_JS},
	{"/core/websock.js",	"JS",	IDR_WEBVIEWER_CORE_WEBSOCK_JS},
	{"/index.html",	"HTML",	IDR_WEBVIEWER_INDEX_HTML},
	{"/",	"HTML",	IDR_WEBVIEWER_INDEX_HTML},
	{"/webviewer/",	"HTML",	IDR_WEBVIEWER_INDEX_HTML},
	{"/webviewer/index.html",	"HTML",	IDR_WEBVIEWER_INDEX_HTML},
	{"/vendor/pako/lib/utils/common.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_UTILS_COMMON_JS},
	{"/vendor/pako/lib/zlib/adler32.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_ADLER32_JS},
	{"/vendor/pako/lib/zlib/constants.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_CONSTANTS_JS},
	{"/vendor/pako/lib/zlib/crc32.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_CRC32_JS},
	{"/vendor/pako/lib/zlib/deflate.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_DEFLATE_JS},
	{"/vendor/pako/lib/zlib/gzheader.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_GZHEADER_JS},
	{"/vendor/pako/lib/zlib/inffast.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_INFFAST_JS},
	{"/vendor/pako/lib/zlib/inflate.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_INFLATE_JS},
	{"/vendor/pako/lib/zlib/inftrees.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_INFTREES_JS},
	{"/vendor/pako/lib/zlib/messages.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_MESSAGES_JS},
	{"/vendor/pako/lib/zlib/trees.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_TREES_JS},
	{"/vendor/pako/lib/zlib/zstream.js",	"JS",	IDR_WEBVIEWER_VENDOR_PAKO_LIB_ZLIB_ZSTREAM_JS},
};





const int filemappingsize = 58;

// -------------------------------------------------------------------------
// WebSocket helpers: handshake (SHA-1 + Base64) and framing.
// The HTTP server uses these to tunnel the browser's binary VNC stream to
// the local RFB port, replacing the obsolete Java applet.
// -------------------------------------------------------------------------

static const char base64Chars[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void WebBase64Encode(const unsigned char* in, size_t len, char* out)
{
	size_t i = 0, j = 0;
	while (i < len) {
		size_t start = i;
		unsigned char b0 = in[i++];
		unsigned char b1 = (i < len) ? in[i++] : 0;
		unsigned char b2 = (i < len) ? in[i++] : 0;
		out[j++] = base64Chars[(b0 >> 2) & 0x3F];
		out[j++] = base64Chars[((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)];
		out[j++] = (i - start >= 2) ? base64Chars[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';
		out[j++] = (i - start == 3) ? base64Chars[b2 & 0x3F] : '=';
	}
	out[j] = 0;
}

// Simple big-endian 32-bit store (SHA-1 length words are big-endian).
static inline void PUT32(unsigned char* p, unsigned v) {
	p[0] = (unsigned char)(v >> 24); p[1] = (unsigned char)(v >> 16);
	p[2] = (unsigned char)(v >> 8); p[3] = (unsigned char)(v);
}
static inline unsigned ROL(unsigned v, unsigned n) { return (v << n) | (v >> (32 - n)); }

static void WebSHA1Hash(const unsigned char* data, size_t len, unsigned char hash[20])
{
	unsigned h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
	unsigned char buf[64];
	size_t idx = 0;
	uint64_t bitlen = ((uint64_t)len) * 8;

	// Process full 64-byte blocks.
	while (len >= 64) {
		memcpy(buf, data + idx, 64);
		idx += 64; len -= 64;
		unsigned w[80];
		for (int i = 0; i < 16; i++)
			w[i] = (buf[i * 4] << 24) | (buf[i * 4 + 1] << 16) | (buf[i * 4 + 2] << 8) | buf[i * 4 + 3];
		for (int i = 16; i < 80; i++)
			w[i] = ROL(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

		unsigned a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
		for (int i = 0; i < 80; i++) {
			unsigned f, k;
			if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
			else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
			else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
			else { f = b ^ c ^ d; k = 0xCA62C1D6; }
			unsigned t = ROL(a, 5) + f + e + k + w[i];
			e = d; d = c; c = ROL(b, 30); b = a; a = t;
		}
		h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
	}

	// Final block with padding.
	memset(buf, 0, 64);
	memcpy(buf, data + idx, len);
	buf[len] = 0x80;
	if (len >= 56) {
		// Need an extra block for length.
		unsigned w[80];
		for (int i = 0; i < 16; i++)
			w[i] = (buf[i * 4] << 24) | (buf[i * 4 + 1] << 16) | (buf[i * 4 + 2] << 8) | buf[i * 4 + 3];
		for (int i = 16; i < 80; i++)
			w[i] = ROL(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
		unsigned a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
		for (int i = 0; i < 80; i++) {
			unsigned f, k;
			if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
			else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
			else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
			else { f = b ^ c ^ d; k = 0xCA62C1D6; }
			unsigned t = ROL(a, 5) + f + e + k + w[i];
			e = d; d = c; c = ROL(b, 30); b = a; a = t;
		}
		h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
		memset(buf, 0, 56);
	}
	PUT32(buf + 56, (unsigned)(bitlen >> 32));
	PUT32(buf + 60, (unsigned)bitlen);

	{
		unsigned w[80];
		for (int i = 0; i < 16; i++)
			w[i] = (buf[i * 4] << 24) | (buf[i * 4 + 1] << 16) | (buf[i * 4 + 2] << 8) | buf[i * 4 + 3];
		for (int i = 16; i < 80; i++)
			w[i] = ROL(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
		unsigned a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
		for (int i = 0; i < 80; i++) {
			unsigned f, k;
			if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
			else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
			else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
			else { f = b ^ c ^ d; k = 0xCA62C1D6; }
			unsigned t = ROL(a, 5) + f + e + k + w[i];
			e = d; d = c; c = ROL(b, 30); b = a; a = t;
		}
		h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
	}

	for (int i = 0; i < 5; i++) PUT32(hash + i * 4, h[i]);
}

static BOOL PerformWebSocketHandshake(VSocket* socket, const char* headers)
{
	const char* key = strstr(headers, "Sec-WebSocket-Key:");
	if (!key) key = strstr(headers, "sec-websocket-key:");
	if (!key) return FALSE;
	key += 18;
	while (*key == ' ' || *key == '\t' || *key == ':') key++;
	const char* end = key;
	while (*end && *end != '\r' && *end != '\n') end++;
	if (end == key) return FALSE;

	char wsKey[128];
	size_t keyLen = (size_t)(end - key);
	if (keyLen > sizeof(wsKey) - 1) keyLen = sizeof(wsKey) - 1;
	memcpy(wsKey, key, keyLen);
	wsKey[keyLen] = 0;

	const char* magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	unsigned char shaInput[128 + 36];
	memcpy(shaInput, wsKey, keyLen);
	memcpy(shaInput + keyLen, magic, 36);
	unsigned char hash[20];
	WebSHA1Hash(shaInput, keyLen + 36, hash);

	char accept[64];
	WebBase64Encode(hash, 20, accept);

	char response[512];
	sprintf_s(response,
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: %s\r\n"
		"\r\n", accept);
	return socket->SendExactHTTP(response, (VCard)strlen(response));
}

static BOOL WebSocketSendFrame(VSocket* socket, unsigned char opcode, const char* payload, int len)
{
	char hdr[10];
	int hdrLen = 2;
	hdr[0] = 0x80 | (opcode & 0x0f); // FIN + opcode
	if (len < 126) {
		hdr[1] = (char)len;
	} else if (len < 65536) {
		hdr[1] = 126;
		hdr[2] = (char)(len >> 8);
		hdr[3] = (char)len;
		hdrLen = 4;
	} else {
		hdr[1] = 127;
		for (int i = 0; i < 8; i++) hdr[2 + i] = (char)(len >> (56 - i * 8));
		hdrLen = 10;
	}
	if (!socket->SendExactHTTP(hdr, hdrLen)) {
		vnclog.Print(LL_CONNERR, VNCLOG("WS send frame: header send failed\n"));
		return FALSE;
	}
	if (len > 0 && !socket->SendExactHTTP(payload, len)) {
		vnclog.Print(LL_CONNERR, VNCLOG("WS send frame: payload send failed\n"));
		return FALSE;
	}
	return TRUE;
}

static BOOL WebSocketRecvFrame(VSocket* socket, char* payload, int maxPayload, int* outLen, CRITICAL_SECTION* sendLock = nullptr)
{
	int totalLen = 0;

	for (;;)
	{
		char hdr[2];
		if (!socket->ReadExactHTTP(hdr, 2)) return FALSE;
		BOOL fin    = (hdr[0] & 0x80) != 0;
		int  opcode = hdr[0] & 0x0f;
		BOOL masked = (hdr[1] & 0x80) != 0;
		uint64_t len = (unsigned char)(hdr[1] & 0x7f);

		char lenBytes[8];
		if (len == 126) {
			if (!socket->ReadExactHTTP(lenBytes, 2)) return FALSE;
			len = ((unsigned char)lenBytes[0] << 8) | (unsigned char)lenBytes[1];
		} else if (len == 127) {
			if (!socket->ReadExactHTTP(lenBytes, 8)) return FALSE;
			len = 0;
			for (int i = 0; i < 8; i++) len = (len << 8) | (unsigned char)lenBytes[i];
		}

		char mask[4] = {};
		if (masked) {
			if (!socket->ReadExactHTTP(mask, 4)) return FALSE;
		}

		// Guard against overflow
		if ((int64_t)len > (int64_t)(maxPayload - totalLen)) return FALSE;

		if (len > 0) {
			if (!socket->ReadExactHTTP(payload + totalLen, (VCard)len)) return FALSE;
			if (masked) {
				for (uint64_t i = 0; i < len; i++)
					payload[totalLen + i] ^= mask[i & 3];
			}
			totalLen += (int)len;
		}

		// Control frames (may arrive mid-fragmentation): handle inline.
		if (opcode == 0x08) {
			if (sendLock) EnterCriticalSection(sendLock);
			WebSocketSendFrame(socket, 0x08, NULL, 0);
			if (sendLock) LeaveCriticalSection(sendLock);
			return FALSE;
		}
		if (opcode == 0x09) {
			// Pong the ping, then keep reading the current message
			const char* pingPayload = payload + (totalLen - (int)len);
			if (sendLock) EnterCriticalSection(sendLock);
			WebSocketSendFrame(socket, 0x0A, pingPayload, (int)len);
			if (sendLock) LeaveCriticalSection(sendLock);
			totalLen -= (int)len; // discard ping payload from buffer
			continue;
		}
		if (opcode == 0x0A) {
			// Unsolicited pong — discard and keep reading
			totalLen -= (int)len;
			continue;
		}

		// If FIN is set this is the last (or only) fragment
		if (fin) break;

		// Continuation frame expected next (opcode == 0x00 for continuations)
	}

	*outLen = totalLen;
	return TRUE;
}

static BOOL WebSocketSendBinary(VSocket* socket, const char* data, int len)
{
	return WebSocketSendFrame(socket, 0x02, data, len);
}

// Forward declarations
static void BridgeWebSocketToRFB(VSocket* clientSocket, vncServer* server);

static const char* GetResourceMimeType(const char* resType)
{
	if (_stricmp(resType, "HTML") == 0) return "text/html";
	if (_stricmp(resType, "CSS") == 0) return "text/css";
	if (_stricmp(resType, "JS") == 0) return "application/javascript";
	return "application/octet-stream";
}

static LPCTSTR GetResourceType(const char* resType)
{
	if (_stricmp(resType, "HTML") == 0) return RT_HTML;
	return resType;
}

// The function for the spawned thread to run
class vncHTTPConnectThread : public omni_thread
{
public:
	vncHTTPConnectThread() : m_shutdown(FALSE), m_tlsEnabled(false), m_server(nullptr), m_socket(nullptr)
	{ memset(m_tlsCertThumbprint, 0, sizeof(m_tlsCertThumbprint)); }

	// Init routine
	virtual BOOL Init(VSocket *socket, vncServer *server);
	void Inithttp(vncServer* svr) { m_server = svr; }

	// Code to be executed by the thread
	virtual void *run_undetached(void * arg);
	// Routines to handle HTTP requests
	virtual void DoHTTP(VSocket *socket);
	virtual char *ReadLine(VSocket *socket, char delimiter);


	// Fields used internally
	BOOL		m_shutdown;

	// TLS mode: if true, perform SChannel handshake before HTTP
	bool		m_tlsEnabled;
	char		m_tlsCertThumbprint[41];
protected:

	vncServer	*m_server;
	VSocket		*m_socket;
};


// Added for HTTP-via-RFB. This function is called when a connection is
// accepted on the RFB port. If the client sends an HTTP request we
// handle it here and return TRUE. Otherwise we return
// FALSE and the caller continues with the RFB handshake.
VBool maybeHandleHTTPRequest(VSocket* sock,vncServer* svr)
{
	if (!sock->ReadSelect(2000)) return false;

	// Client is sending data. Create a vncHTTPConnectThread to
	// handle it.
	vncHTTPConnectThread http;
	http.Inithttp(svr);
	http.DoHTTP(sock);
	sock->Shutdown();
	sock->Close();
	delete sock;
	return true;
}


// Method implementations
BOOL vncHTTPConnectThread::Init(VSocket *socket, vncServer *server)
{
	// Save the server pointer
	m_server = server;

	// Save the socket pointer
	m_socket = socket;

	// Start the thread (TLS fields may already be set by InitTLS before this call)
	m_shutdown = FALSE;
	start_undetached();

	return TRUE;
}

// Code to be executed by the thread
void *vncHTTPConnectThread::run_undetached(void * arg)
{

	// Go into a loop, listening for connections on the given socket
	VSocket* new_socket = NULL;
	while (!m_shutdown)
	{
		// Accept an incoming connection
		VSocket *new_socket = m_socket->Accept();
		if (new_socket == NULL)
			break;
		if (m_shutdown)
		{
			delete new_socket;
			break;
		}
		vnclog.Print(LL_CLIENTS, VNCLOG("HTTP client connected\n"));
		// Successful accept - perform the transaction
		new_socket->SetTimeout(15000); //ms

		if (m_tlsEnabled)
		{
			// Perform TLS handshake before HTTP processing
			SchannelTLS tls;
			bool certOk = false;
			if (m_tlsCertThumbprint[0])
				certOk = tls.LoadCertByThumbprint(m_tlsCertThumbprint);
			else
			{
				char thumbprint[41];
				certOk = tls.EnsureSelfSignedCert(thumbprint);
				if (certOk)
				{
					settings->setTLSCertThumbprint(thumbprint);
					settings->save();
					strncpy_s(m_tlsCertThumbprint, thumbprint, 40);
				}
			}

			if (certOk && tls.ServerHandshake(new_socket))
			{
				// Install TLS callbacks so all HTTP/WebSocket I/O is encrypted
				VSocket::TLSCallbacks cb;
				cb.send = [&tls, new_socket](const char* b, int l) -> bool {
					return tls.Send(new_socket, b, l);
				};
				cb.recv = [&tls, new_socket](char* b, int l, int* got) -> bool {
					return tls.Recv(new_socket, b, l, got);
				};
				new_socket->SetTLSCallbacks(&cb);

				DoHTTP(new_socket);

				new_socket->SetTLSCallbacks(nullptr);
				tls.Shutdown(new_socket);
			}
			else
			{
				vnclog.Print(LL_CONNERR, VNCLOG("TLS: handshake failed, dropping connection\n"));
			}
		}
		else
		{
			DoHTTP(new_socket);
		}

		// And close the client
		new_socket->Shutdown();
		new_socket->Close();
		delete new_socket;
		new_socket = NULL;
	}
	if (new_socket)
		delete new_socket;

	return NULL;
}

struct WebSocketBridgeContext {
	VSocket* client;
	VSocket* rfb;
	volatile BOOL running;
	CRITICAL_SECTION sendLock;
};

static unsigned __stdcall WSClientToRFB(void* arg)
{
	WebSocketBridgeContext* ctx = (WebSocketBridgeContext*)arg;
	static const int WS_MAX_PAYLOAD = 256 * 1024;
	std::vector<char> payloadBuf(WS_MAX_PAYLOAD);
	char* payload = payloadBuf.data();
	while (ctx->running) {
		int len = 0;
		if (!WebSocketRecvFrame(ctx->client, payload, WS_MAX_PAYLOAD, &len, &ctx->sendLock)) {
			ctx->running = FALSE;
			break;
		}
		if (len > 0) {
			if (!ctx->rfb->SendExact(payload, len)) {
				vnclog.Print(LL_CONNERR, VNCLOG("WS->RFB: SendExact failed\n"));
				ctx->running = FALSE;
				break;
			}
		}
	}
	return 0;
}

static unsigned __stdcall WSRFBToClient(void* arg)
{
	WebSocketBridgeContext* ctx = (WebSocketBridgeContext*)arg;
	static const int RFB_BUF = 256 * 1024;
	std::vector<char> buf(RFB_BUF);
	while (ctx->running) {
		VInt r = ctx->rfb->Read(buf.data(), RFB_BUF);
		if (r <= 0) {
				ctx->running = FALSE;
			break;
		}
		EnterCriticalSection(&ctx->sendLock);
		BOOL ok = WebSocketSendBinary(ctx->client, buf.data(), r);
		LeaveCriticalSection(&ctx->sendLock);
		if (!ok) {
			vnclog.Print(LL_CONNERR, VNCLOG("RFB->WS: WebSocketSendBinary failed\n"));
			ctx->running = FALSE;
			break;
		}
	}
	return 0;
}

static void BridgeWebSocketToRFB(VSocket* clientSocket, vncServer* server)
{
	// Connect a second socket to the local RFB listener so that the
	// browser's WebSocket stream is forwarded to the VNC server.
	VSocket* rfbSocket = new VSocket();
	if (!rfbSocket->CreateConnect("127.0.0.1", settings->getPortNumber())) {
		vnclog.Print(LL_CONNERR, VNCLOG("failed to connect WebSocket bridge to RFB port %d\n"), settings->getPortNumber());
		delete rfbSocket;
		return;
	}

	WebSocketBridgeContext ctx;
	ctx.client = clientSocket;
	ctx.rfb = rfbSocket;
	ctx.running = TRUE;
	InitializeCriticalSection(&ctx.sendLock);

	unsigned int tid1 = 0, tid2 = 0;
	HANDLE h1 = (HANDLE)_beginthreadex(NULL, 0, WSClientToRFB, &ctx, 0, &tid1);
	HANDLE h2 = (HANDLE)_beginthreadex(NULL, 0, WSRFBToClient, &ctx, 0, &tid2);

	if (h1 && h2) {
		HANDLE handles[2] = { h1, h2 };
		// Wait for either direction to fail.
		DWORD waitResult = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		// Force the other direction out of its read by shutting both sockets down.
		clientSocket->Shutdown();
		rfbSocket->Shutdown();
		// Wait for both threads to finish.
		WaitForMultipleObjects(2, handles, TRUE, INFINITE);
	} else {
		vnclog.Print(LL_CONNERR, VNCLOG("BridgeWebSocketToRFB: failed to create bridge threads\n"));
	}
	if (h1) CloseHandle(h1);
	if (h2) CloseHandle(h2);

	DeleteCriticalSection(&ctx.sendLock);
	rfbSocket->Close();
	delete rfbSocket;
}

void vncHTTPConnectThread::DoHTTP(VSocket *socket)

{
	char filename[1024];
	char *line;

	vnclog.Print(LL_CLIENTS, VNCLOG("DoHTTP started\n"));

	// Read in the HTTP request line
	if ((line = ReadLine(socket, '\n')) == NULL)
	{
		vnclog.Print(LL_CONNERR, VNCLOG("DoHTTP: ReadLine failed\n"));
		return;
	}

	// Scan the header for the filename and free the storage
	int result = sscanf_s(line, "GET %s ", filename, 1024);
	delete [] line;
	if ((result == 0) || (result == EOF))
	{
		vnclog.Print(LL_CONNERR, VNCLOG("DoHTTP: sscanf failed\n"));
		return;
	}

	vnclog.Print(LL_CLIENTS, VNCLOG("file %s requested\n"), filename);

	// Read in the rest of the browser's request headers. We keep them for
	// WebSocket upgrade detection.
	char headers[4096];
	int headerPos = 0;
	BOOL emptyline = TRUE;

	for (;;)
	{
		char c;

		if (!socket->ReadExactHTTP(&c, 1))
		{
			vnclog.Print(LL_CONNERR, VNCLOG("DoHTTP: header read failed\n"));
			return;
		}
		if (headerPos < (int)sizeof(headers) - 1) {
			headers[headerPos++] = c;
		}
		if (c == '\n')
		{
			if (emptyline)
				break;
			emptyline = TRUE;
		}
		else
			if (c >= ' ')
			{
				emptyline = FALSE;
			}
	}
	headers[headerPos] = 0;


	// Is this a WebSocket upgrade request for the RFB tunnel?
	if (strstr(headers, "Upgrade: websocket") || strstr(headers, "upgrade: websocket"))
	{
		if (strcmp(filename, "/ws") == 0)
		{
			if (PerformWebSocketHandshake(socket, headers))
			{
				vnclog.Print(LL_CLIENTS, VNCLOG("WebSocket tunnel established\n"));
				socket->SetTimeout(0); // No timeout during RFB session (auth needs user input)
				BridgeWebSocketToRFB(socket, m_server);
			}
			else
			{
				vnclog.Print(LL_CONNERR, VNCLOG("WebSocket handshake failed\n"));
			}
			return;
		}
	}

    if (filename[0] != '/')
	{
		vnclog.Print(LL_CONNERR, VNCLOG("filename didn't begin with '/'\n"));
		socket->SendExactHTTP(HTTP_MSG_NOSUCHFILE, (const VCard)strlen(HTTP_MSG_NOSUCHFILE));
		return;
	}

	// If server is not accepting RFB connections, show a simple status page.
	if (strcmp(filename, "/") == 0 && !m_server->SockConnected())
	{
		vnclog.Print(LL_CLIENTS, VNCLOG("DoHTTP: sending disabled page (RFB not listening)\n"));
		if (!socket->SendExactHTTP(HTTP_MSG_OK, (const VCard)strlen(HTTP_MSG_OK)))
			return;
		socket->SendExactHTTP(HTTP_MSG_NOSOCKCONN, (const VCard)strlen(HTTP_MSG_NOSOCKCONN));
		return;
	}

	// File requested was not the index so check the mappings
	// list for a different file.

	if (settings->getRequireMSLogon())
	{

		for (int x=0; x < filemappingsize; x++)
	{
		if (strcmp(filename, filemapping2[x].filename) == 0)
		{
			HRSRC resource;
			HGLOBAL resourcehan;
			char *resourceptr;
			int resourcesize;

			resource = FindResource(hInstResDLL,
					MAKEINTRESOURCE(filemapping2[x].resourceID),
					GetResourceType(filemapping2[x].type)
					);
			if (resource == NULL)
			{
				vnclog.Print(LL_CONNERR, VNCLOG("DoHTTP: resource not found for %s (id=%d type=%s)\n"),
					filemapping2[x].filename, filemapping2[x].resourceID, filemapping2[x].type);
				return;
			}

			resourcesize = SizeofResource(hInstResDLL, resource);

			resourcehan = LoadResource(hInstResDLL, resource);
			if (resourcehan == NULL)
				return;

			resourceptr = (char *)LockResource(resourcehan);
			if (resourceptr == NULL)
				return;

			char responseHdr[512];
			sprintf_s(responseHdr,
				"HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n"
				"Cache-Control: no-cache, no-store, must-revalidate\r\n"
				"Pragma: no-cache\r\n"
				"Expires: 0\r\n\r\n",
				GetResourceMimeType(filemapping2[x].type), resourcesize);
			if (!socket->SendExactHTTP(responseHdr, (const VCard)strlen(responseHdr)))
				return;

			if (!socket->SendExactHTTP(resourceptr, resourcesize))
				return;

			return;
		}
	}
	}
	else
	{
	// Now search the mappings for the desired file
	for (int x=0; x < filemappingsize; x++)
	{
		if (strcmp(filename, filemapping[x].filename) == 0)
		{
			HRSRC resource;
			HGLOBAL resourcehan;
			char *resourceptr;
			int resourcesize;

			resource = FindResource(hInstResDLL,
					MAKEINTRESOURCE(filemapping[x].resourceID),
					GetResourceType(filemapping[x].type)
					);
			if (resource == NULL)
			{
				vnclog.Print(LL_CONNERR, VNCLOG("DoHTTP: resource not found for %s (id=%d type=%s)\n"),
					filemapping[x].filename, filemapping[x].resourceID, filemapping[x].type);
				return;
			}

			resourcesize = SizeofResource(hInstResDLL, resource);

			resourcehan = LoadResource(hInstResDLL, resource);
			if (resourcehan == NULL)
				return;

			resourceptr = (char *)LockResource(resourcehan);
			if (resourceptr == NULL)
				return;

			char responseHdr[512];
			sprintf_s(responseHdr,
				"HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n"
				"Cache-Control: no-cache, no-store, must-revalidate\r\n"
				"Pragma: no-cache\r\n"
				"Expires: 0\r\n\r\n",
				GetResourceMimeType(filemapping[x].type), resourcesize);
			if (!socket->SendExactHTTP(responseHdr, (const VCard)strlen(responseHdr)))
				return;

			if (!socket->SendExactHTTP(resourceptr, resourcesize))
				return;

			return;
		}
	}
	}

	// Send the NoSuchFile notification message to the client
	vnclog.Print(LL_CONNERR, VNCLOG("DoHTTP: sending 404 for %s\n"), filename);
	if (!socket->SendExactHTTP(HTTP_MSG_NOSUCHFILE, (const VCard)strlen(HTTP_MSG_NOSUCHFILE)))
		return;
}

char *vncHTTPConnectThread::ReadLine(VSocket *socket, char delimiter)

{
	int max=1024;
	// Allocate the maximum required buffer
	char *buffer = new char[max+1];
	int buffpos = 0;

	// Read in data until a delimiter is read
	for (;;)
	{
		char c;

		if (!socket->ReadExactHTTP(&c, 1))
		{
			delete [] buffer;
			return NULL;
		}

		if (c == delimiter)
		{
			buffer[buffpos] = 0;
			return buffer;
		}

		buffer[buffpos] = c;
		buffpos++;

		if (buffpos == (max-1))
		{
			buffer[buffpos] = 0;
			return buffer;
		}
	}
}

// The vncSockConnect class implementation

vncHTTPConnect::vncHTTPConnect()
{
	m_thread = NULL;
}

vncHTTPConnect::~vncHTTPConnect()
{
   m_socket.Shutdown();

    // Join with our lovely thread
   if (m_thread != NULL)
   {
	   // *** This is a hack to force the listen thread out of the accept call,
	   // because Winsock accept semantics are broken.
	   ((vncHTTPConnectThread*)m_thread)->m_shutdown = TRUE;

	   VSocket socket;
	   if (settings->getIPV6()) {
		   socket.CreateBindConnect("localhost", m_port);
	   }
	   else {
		   socket.Create();
		   socket.Bind(0);
		   socket.Connect("localhost", m_port);
		}
		socket.Close();

		void *returnval;
		m_thread->join(&returnval);
		m_thread = NULL;

		m_socket.Close();
    }
}

BOOL vncHTTPConnect::Init(vncServer *server, UINT port)
{
	// Save the port id
	m_port = port;
	if (settings->getIPV6()) {
		if (!m_socket.CreateBindListen(m_port, settings->getLoopbackOnly()))
			return FALSE;
	}
	else {
		// Create the listening socket
		if (!m_socket.Create())
			return FALSE;

		// Bind it
		if (!m_socket.Bind(m_port, settings->getLoopbackOnly()))
			return FALSE;

		// Set it to listen
		if (!m_socket.Listen())
			return FALSE;
	}
	// Create the new thread
	m_thread = new vncHTTPConnectThread;
	if (m_thread == NULL)
		return FALSE;

	// And start it running
	return ((vncHTTPConnectThread *)m_thread)->Init(&m_socket, server);
}

BOOL vncHTTPConnect::InitTLS(vncServer *server, UINT port, const char* certThumbprint)
{
	m_port = port;
	if (settings->getIPV6()) {
		if (!m_socket.CreateBindListen(m_port, settings->getLoopbackOnly()))
			return FALSE;
	}
	else {
		if (!m_socket.Create())
			return FALSE;
		if (!m_socket.Bind(m_port, settings->getLoopbackOnly()))
			return FALSE;
		if (!m_socket.Listen())
			return FALSE;
	}
	m_thread = new vncHTTPConnectThread;
	if (m_thread == NULL)
		return FALSE;

	vncHTTPConnectThread* t = (vncHTTPConnectThread*)m_thread;
	t->m_tlsEnabled = true;
	if (certThumbprint && certThumbprint[0])
		strncpy_s(t->m_tlsCertThumbprint, certThumbprint, 40);
	else
		memset(t->m_tlsCertThumbprint, 0, sizeof(t->m_tlsCertThumbprint));

	return t->Init(&m_socket, server);
}
