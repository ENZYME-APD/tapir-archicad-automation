#pragma once

#include "UniString.hpp"

// Opens an http(s) link in the default browser, without a shell, so no character of it is interpreted.
void OpenUrl (const GS::UniString& url);

// Percent-encodes a path segment or query value (RFC 3986: only the unreserved characters are kept).
GS::UniString EncodeUrlComponent (const GS::UniString& text);
