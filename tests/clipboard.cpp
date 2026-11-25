#include "common/Clipboard.h"
#include "rdr/ZlibOutStream.h"
#include <zlib.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

static int checks = 0;
#define CHECK(value) do { ++checks; if (!(value)) throw std::runtime_error(#value); } while (0)

static void bytes(BYTE*& target, int& length, const char* value)
{
	length = (int)strlen(value) + 1;
	target = new BYTE[length];
	memcpy(target, value, length);
}

static void sample(ClipboardData& data)
{
	bytes(data.m_pDataText, data.m_lengthText, u8"Euro \u20ac, Cyrillic \u0416, CJK \u4e2d, emoji \U0001f600\r\n");
	bytes(data.m_pDataRTF, data.m_lengthRTF, "{\\rtf1 test}");
	bytes(data.m_pDataHTML, data.m_lengthHTML, "Version:1.0\r\n<html>test</html>");
	bytes(data.m_pDataDIB, data.m_lengthDIB, "synthetic DIB payload");
	data.m_crc = crc32(0, data.m_pDataText, data.m_lengthText);
}

static void receive(ExtendedClipboardDataMessage& message)
{
	int length = message.GetDataLength();
	CHECK(message.SetReceivedLength(length));
}

// Inflates a clipProvide payload the way UltraVNC 1.0.9.x rdr::ZlibInStream does:
// every inflate call must return Z_OK, so a finished stream (Z_STREAM_END) is fatal.
static size_t legacyInflate(ExtendedClipboardDataMessage& message, BYTE* raw, size_t size)
{
	z_stream stream = {};
	CHECK(inflateInit(&stream) == Z_OK);
	stream.next_in = (Bytef*)message.GetData() + 4;
	stream.avail_in = message.GetDataLength() - 4;
	stream.next_out = raw;
	stream.avail_out = (uInt)size;
	int result = inflate(&stream, Z_SYNC_FLUSH);
	size_t length = stream.total_out;
	bool consumed = stream.avail_in == 0;
	inflateEnd(&stream);
	CHECK(result == Z_OK && consumed);
	return length;
}

static void caps(ClipboardSettings& settings, CARD32 flags, CARD32 text = 0, CARD32 rtf = 0, CARD32 html = 0, CARD32 dib = 0)
{
	ExtendedClipboardDataMessage message;
	message.AddFlag(clipCaps | flags);
	CARD32 sizes[] = { text, rtf, html, dib };
	for (int i = 0; i < 16; ++i)
		if (flags & (1u << i))
			message.AppendInt(i < 4 ? sizes[i] : 123);
	receive(message);
	CHECK(settings.HandleCapsPacket(message));
}

static void payload(ExtendedClipboardDataMessage& message, CARD32 formats, const BYTE* plain, size_t length, bool historical = false)
{
	message.AddFlag(clipProvide | formats);
	if (historical) {
		rdr::MemOutStream output;
		{
			rdr::ZlibOutStream stream(&output, 0, 9);
			stream.writeBytes(plain, (int)length);
			stream.flush();
		}
		message.AppendBytes((BYTE*)output.data(), output.length());
	} else {
		uLongf size = compressBound((uLong)length);
		std::vector<BYTE> compressed(size);
		CHECK(compress2(compressed.data(), &size, plain, (uLong)length, 9) == Z_OK);
		message.AppendBytes(compressed.data(), (int)size);
	}
	receive(message);
}

static void testCaps()
{
	ClipboardSettings settings(ClipboardSettings::defaultViewerCaps);
	CHECK(settings.m_remoteCaps == (clipText | clipRTF | clipHTML | clipRequest | clipNotify | clipProvide));
	CHECK(settings.m_nRequestedLimitText == 20 * 1024 * 1024);
	CHECK(!settings.m_nRequestedLimitRTF && !settings.m_nRequestedLimitHTML && !settings.m_nRequestedLimitDIB);
	settings.m_nLimitRTF = 0;
	caps(settings, clipText | clipRTF | clipDIB | clipProvide | clipRequest | (1u << 5), 0xffffffffu, 7, 0, 99);
	CHECK(settings.m_nRequestedLimitText == 0xffffffffu);
	CHECK(settings.m_nRequestedLimitRTF == 7 && settings.m_nRequestedLimitDIB == 99);
	CHECK(settings.m_nLimitText == ClipboardSettings::defaultLimitText && settings.m_nLimitRTF == 0);
	settings.m_myCaps = clipCaps | clipText | clipHTML | clipProvide;
	ExtendedClipboardDataMessage message;
	settings.PrepareCapsPacket(message);
	CHECK(message.GetDataLength() == 12);
	receive(message);
	CHECK(message.ReadInt() == ClipboardSettings::defaultLimitText);
	CHECK(message.ReadInt() == ClipboardSettings::defaultLimitHTML);
	caps(settings, clipText | clipProvide, 13);
	CHECK(settings.m_nLimitRTF == 0 && settings.m_nRequestedLimitRTF == 0);
}

static void testValidation()
{
	ExtendedClipboardDataMessage message;
	CHECK(!message.SetReceivedLength(0));
	CHECK(!message.SetReceivedLength(3));
	CHECK(!message.SetReceivedLength(ExtendedClipboardDataMessage::maxPayload + 1));
	message.AddFlag(clipCaps | clipText);
	CHECK(!message.SetReceivedLength(4));
	message.AppendInt(0);
	receive(message);
	message.Reset();
	message.AddFlag(clipRequest | clipNotify);
	CHECK(!message.SetReceivedLength(4));
	message.Reset();
	message.AddFlag(clipPeek);
	receive(message);
	message.AppendInt(0);
	CHECK(!message.SetReceivedLength(8));
	message.Reset();
	message.AddFlag(clipProvide);
	CHECK(!message.SetReceivedLength(4));
}

static void testRequests()
{
	Clipboard clipboard(ClipboardSettings::defaultServerCaps);
	clipboard.settings.m_bSupportsEx = true;
	ClipboardData data;
	sample(data);
	caps(clipboard.settings, clipText | clipRTF | clipHTML | clipDIB | clipProvide | clipNotify | clipRequest);
	CHECK(clipboard.UpdateClipTextEx(data, clipRequest | clipText));
	CHECK(clipboard.m_bNeedToProvide && !clipboard.m_bNeedToNotify);
	CHECK(clipboard.extendedClipboardDataMessage.GetFlags() == (clipProvide | clipText));
	receive(clipboard.extendedClipboardDataMessage);
	ClipboardData decoded;
	CHECK(decoded.Decode(clipboard.extendedClipboardDataMessage));
	CHECK(decoded.Formats() == clipText);
	CHECK(decoded.m_lengthText == data.m_lengthText);
	CHECK(!memcmp(decoded.m_pDataText, data.m_pDataText, data.m_lengthText));
	int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)decoded.m_pDataText, decoded.m_lengthText, NULL, 0);
	std::vector<wchar_t> wide(count);
	CHECK(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)decoded.m_pDataText, decoded.m_lengthText, wide.data(), count) == count);
	CHECK(!wcscmp(wide.data(), L"Euro \u20ac, Cyrillic \u0416, CJK \u4e2d, emoji \U0001f600\r\n"));
	std::vector<char> roundtrip(data.m_lengthText);
	CHECK(WideCharToMultiByte(CP_UTF8, 0, wide.data(), count, roundtrip.data(), (int)roundtrip.size(), NULL, NULL) == data.m_lengthText);
	CHECK(!memcmp(roundtrip.data(), data.m_pDataText, data.m_lengthText));
	CHECK(clipboard.UpdateClipTextEx(data, clipRequest | clipRTF | clipHTML | clipDIB));
	CHECK(clipboard.extendedClipboardDataMessage.GetFlags() == (clipProvide | clipRTF | clipHTML | clipDIB));
	receive(clipboard.extendedClipboardDataMessage);
	CHECK(decoded.Decode(clipboard.extendedClipboardDataMessage));
	CHECK(decoded.Formats() == (clipRTF | clipHTML | clipDIB));
	ClipboardData empty;
	CHECK(clipboard.UpdateClipTextEx(empty, clipRequest | clipText));
	CHECK(clipboard.extendedClipboardDataMessage.GetFlags() == clipProvide);
	receive(clipboard.extendedClipboardDataMessage);
	CHECK(decoded.Decode(clipboard.extendedClipboardDataMessage) && !decoded.Formats());
}

static void testLimitsAndOldPeers()
{
	Clipboard clipboard(ClipboardSettings::defaultViewerCaps);
	clipboard.settings.m_bSupportsEx = true;
	ClipboardData data;
	sample(data);
	caps(clipboard.settings, clipText | clipProvide | clipRequest | clipPeek, data.m_lengthText);
	CHECK(clipboard.UpdateClipTextEx(data));
	CHECK(clipboard.m_bNeedToProvide && !clipboard.m_bNeedToNotify);
	CHECK(clipboard.extendedClipboardDataMessage.GetFlags() == (clipProvide | clipText));
	caps(clipboard.settings, clipText | clipProvide | clipRequest | clipPeek, data.m_lengthText - 1);
	CHECK(!clipboard.UpdateClipTextEx(data, clipProvide | clipText));
	CHECK(!clipboard.m_bNeedToProvide && !clipboard.m_bNeedToNotify);
	caps(clipboard.settings, clipText | clipProvide | clipNotify | clipRequest);
	CHECK(clipboard.UpdateClipTextEx(data, clipProvide | clipText));
	CHECK(!clipboard.m_bNeedToProvide && clipboard.m_bNeedToNotify);
	CHECK(clipboard.extendedClipboardDataNotifyMessage.HasFlag(clipNotify));
	caps(clipboard.settings, clipText | clipRTF | clipHTML | clipDIB | clipProvide | clipNotify | clipRequest,
		0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu);
	clipboard.settings.m_nLimitDIB = 1;
	CHECK(clipboard.UpdateClipTextEx(data, clipProvide));
	CHECK(clipboard.extendedClipboardDataMessage.GetFlags() == (clipProvide | clipText | clipRTF | clipHTML | clipDIB));
	BYTE raw[4096];
	size_t length = legacyInflate(clipboard.extendedClipboardDataMessage, raw, sizeof(raw));
	CHECK(length == 16 + data.m_lengthText + data.m_lengthRTF + data.m_lengthHTML + data.m_lengthDIB);
	caps(clipboard.settings, clipText | clipNotify | clipRequest);
	CHECK(clipboard.UpdateClipTextEx(data, clipProvide));
	CHECK(!clipboard.m_bNeedToProvide && clipboard.m_bNeedToNotify);
}

static void testPeekAndChanges()
{
	Clipboard clipboard(ClipboardSettings::defaultViewerCaps);
	clipboard.settings.m_bSupportsEx = true;
	ClipboardData data;
	sample(data);
	CHECK(clipboard.UpdateClipTextEx(data));
	CHECK(!clipboard.UpdateClipTextEx(data));
	data.m_pDataHTML[0] ^= 1;
	CHECK(clipboard.UpdateClipTextEx(data));
	data.m_pDataRTF[0] ^= 1;
	CHECK(clipboard.UpdateClipTextEx(data));
	data.m_pDataDIB[0] ^= 1;
	CHECK(clipboard.UpdateClipTextEx(data));
	CHECK(clipboard.UpdateClipTextEx(data, clipPeek));
	CHECK(clipboard.extendedClipboardDataNotifyMessage.GetFlags() == (clipNotify | clipText | clipRTF | clipHTML | clipDIB));
	clipboard.extendedClipboardDataNotifyMessage.Reset();
	ClipboardData empty;
	CHECK(clipboard.UpdateClipTextEx(empty, clipPeek));
	CHECK(clipboard.extendedClipboardDataNotifyMessage.GetFlags() == clipNotify);
	CHECK(clipboard.UpdateClipTextEx(empty));
	CHECK(clipboard.m_bNeedToNotify && !clipboard.m_bNeedToProvide);
	clipboard.AcceptRemoteData(data);
	CHECK(!clipboard.UpdateClipTextEx(data));
	CHECK(clipboard.RequestFormats(clipText | clipRTF | clipDIB, true) == (clipText | clipRTF));
	CHECK(!clipboard.RequestFormats(clipText | clipRTF, true));
	CHECK(clipboard.RequestFormats(clipDIB, false) == clipDIB);
	clipboard.AcceptRemoteData(data);
	CHECK(!clipboard.m_requestedRemoteFormats && !clipboard.m_notifiedRemoteFormats);
}

static void testPayloads()
{
	const BYTE text[] = { 0, 0, 0, 4, 'a', '\r', '\n', 0 };
	for (int historical = 0; historical < 2; ++historical) {
		ExtendedClipboardDataMessage message;
		payload(message, clipText, text, sizeof(text), historical != 0);
		ClipboardData data;
		CHECK(data.Decode(message));
		CHECK(data.m_lengthText == 4 && !memcmp(data.m_pDataText, text + 4, 4));
	}
	const BYTE future[] = { 0, 0, 0, 2, 'x', 0, 0, 0, 0, 3, 1, 2, 3 };
	ExtendedClipboardDataMessage unknown;
	payload(unknown, clipText | (1u << 5), future, sizeof(future));
	ClipboardData data;
	CHECK(data.Decode(unknown));
	CHECK(data.Formats() == clipText);
	const std::vector<std::vector<BYTE>> invalid = {
		{ 0, 0, 0 }, { 0xff, 0xff, 0xff, 0xff }, { 0, 0, 0, 2, 'x' },
		{ 0, 0, 0, 1, 'x' }, { 0, 0, 0, 2, 0xff, 0 }, { 0, 0, 0, 3, 0, 'x', 0 },
		{ 0, 0, 0, 0 }, { 0, 0, 0, 1, 0, 'x' }
	};
	for (const auto& bytes : invalid) {
		ExtendedClipboardDataMessage message;
		payload(message, clipText, bytes.data(), bytes.size());
		CHECK(!data.Decode(message));
		CHECK(!data.Restore(NULL, message));
	}
	ExtendedClipboardDataMessage truncated;
	payload(truncated, clipText, text, sizeof(text));
	int length = truncated.GetRemainingLength() + 4;
	CHECK(truncated.SetReceivedLength(length - 1));
	CHECK(!data.Decode(truncated));
	ExtendedClipboardDataMessage corrupt;
	payload(corrupt, clipText, text, sizeof(text));
	corrupt.GetBuffer()[5] ^= 0xff;
	CHECK(!data.Decode(corrupt));
	ExtendedClipboardDataMessage historical;
	payload(historical, clipText, text, sizeof(text), true);
	length = historical.GetRemainingLength() + 4;
	CHECK(historical.SetReceivedLength(length - 1));
	CHECK(!data.Decode(historical));
}

static void testPolicyAndChunks()
{
	Clipboard clipboard(ClipboardSettings::defaultViewerCaps);
	clipboard.settings.m_bSupportsEx = true;
	clipboard.settings.m_nLimitRTF = 0;
	clipboard.settings.m_nLimitHTML = 0;
	ClipboardData data;
	sample(data);
	caps(clipboard.settings, clipText | clipRTF | clipHTML | clipDIB | clipProvide | clipNotify | clipRequest);
	CHECK(clipboard.UpdateClipTextEx(data));
	CHECK(clipboard.extendedClipboardDataNotifyMessage.GetFlags() == (clipNotify | clipText));
	CHECK(clipboard.UpdateClipTextEx(data, clipRequest | clipRTF));
	CHECK(clipboard.extendedClipboardDataMessage.GetFlags() == (clipProvide | clipRTF));
	int length = clipboard.extendedClipboardDataMessage.GetDataLength();
	CHECK(clipboard.UpdateClipTextEx(data, clipPeek));
	CHECK(clipboard.m_bNeedToProvide);
	CHECK(clipboard.extendedClipboardDataMessage.GetDataLength() == length);
	CHECK(clipboard.extendedClipboardDataNotifyMessage.GetFlags() == (clipNotify | data.Formats()));
	clipboard.AcceptRemoteData(data);
	CHECK(!clipboard.m_bNeedToProvide && !clipboard.m_bNeedToNotify);
	caps(clipboard.settings, clipText | clipRTF);
	CHECK(!clipboard.UpdateClipTextEx(data, clipProvide));
	CHECK(!clipboard.RequestFormats(clipText, false));
	const int size = 1024 * 1024;
	std::vector<BYTE> plain(size + 4);
	plain[1] = 0x10;
	for (int i = 0; i < size; ++i)
		plain[i + 4] = (BYTE)(i * 31);
	for (int old = 0; old < 2; ++old) {
		ExtendedClipboardDataMessage message;
		payload(message, clipRTF, plain.data(), plain.size(), old != 0);
		ClipboardData decoded;
		CHECK(decoded.Decode(message));
		CHECK(decoded.m_lengthRTF == size);
		CHECK(!memcmp(decoded.m_pDataRTF, plain.data() + 4, size));
	}
}

static void testLegacyServerNotifyRequest()
{
	// UltraVNC 1.0.9.x servers advertise clipPeek but not clipNotify, and send a
	// clipProvide + clipNotify pair on every clipboard change. The viewer answers
	// the notify with a clipRequest (dispatched on the UI thread) and accepts the
	// provide that follows.
	Clipboard viewer(ClipboardSettings::defaultViewerCaps);
	viewer.settings.m_bSupportsEx = true;
	caps(viewer.settings, clipText | clipRTF | clipHTML | clipDIB | clipProvide | clipRequest | clipPeek,
		0x00A00000u, 0x00200000u, 0x00200000u, 0);
	CHECK(!(viewer.settings.m_remoteCaps & clipNotify));

	CARD32 notified = clipText | clipRTF | clipHTML;
	viewer.m_notifiedRemoteFormats = notified;
	viewer.m_requestedRemoteFormats = 0;
	CHECK(viewer.RequestFormats(notified, true) == notified);
	CHECK(!viewer.RequestFormats(notified, true));

	ClipboardData data;
	sample(data);
	viewer.AcceptRemoteData(data);
	CHECK(!viewer.m_requestedRemoteFormats && !viewer.m_notifiedRemoteFormats);
	CHECK(viewer.RequestFormats(notified, true) == notified);

	// A legacy server never wants notifies back, so viewer provides never pair
	// with a clipNotify even when formats are withheld by limits.
	viewer.settings.m_nLimitRTF = 0;
	viewer.settings.m_nLimitHTML = 0;
	CHECK(viewer.UpdateClipTextEx(data, clipRequest | clipText));
	CHECK(viewer.m_bNeedToProvide && !viewer.m_bNeedToNotify);
	BYTE raw[4096];
	CHECK(legacyInflate(viewer.extendedClipboardDataMessage, raw, sizeof(raw)) == 4 + (size_t)data.m_lengthText);

	// Copying text on the viewer side sends an automatic clipProvide that a
	// 1.0.9.x server must be able to inflate without hitting Z_STREAM_END.
	ClipboardData copied;
	bytes(copied.m_pDataText, copied.m_lengthText, "copied from Notepad++");
	copied.m_crc = crc32(0, copied.m_pDataText, copied.m_lengthText);
	CHECK(viewer.UpdateClipTextEx(copied));
	CHECK(viewer.m_bNeedToProvide);
	CHECK(viewer.extendedClipboardDataMessage.GetFlags() == (clipProvide | clipText));
	CHECK(legacyInflate(viewer.extendedClipboardDataMessage, raw, sizeof(raw)) == 4 + (size_t)copied.m_lengthText);
	receive(viewer.extendedClipboardDataMessage);
	ClipboardData decoded;
	CHECK(decoded.Decode(viewer.extendedClipboardDataMessage));
	CHECK(decoded.m_lengthText == copied.m_lengthText);
}

static void testLegacy()
{
	Clipboard clipboard(ClipboardSettings::defaultServerCaps);
	ClipboardData data;
	sample(data);
	CHECK(!clipboard.settings.m_bSupportsEx);
	CHECK(clipboard.UpdateClipTextEx(data));
	CHECK(clipboard.m_crc == data.m_crc);
	CHECK(!clipboard.m_strLastCutText.empty());
	CHECK(!clipboard.UpdateClipTextEx(data));
	data.m_pDataHTML[0] ^= 1;
	CHECK(!clipboard.UpdateClipTextEx(data));
}

int main()
{
	try {
		testCaps();
		testValidation();
		testRequests();
		testLimitsAndOldPeers();
		testPeekAndChanges();
		testPayloads();
		testPolicyAndChunks();
		testLegacyServerNotifyRequest();
		testLegacy();
		printf("PASS: %d clipboard checks\n", checks);
		return 0;
	} catch (const std::exception& error) {
		fprintf(stderr, "FAIL after %d checks: %s\n", checks, error.what());
		return 1;
	}
}
