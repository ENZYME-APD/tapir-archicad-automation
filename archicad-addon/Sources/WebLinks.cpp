#include "WebLinks.hpp"

#include "APIEnvir.h"   // with ACAPinc.h, brings in windows.h, which shellapi.h needs
#include "ACAPinc.h"
#include "Array.hpp"
#include "Process.hpp"

#if defined (WINDOWS)
#pragma warning (push)
#pragma warning (disable : 4995 4091)   // as in the DevKit's Win32ShellInterface.hpp
#include <shellapi.h>
#pragma warning (pop)
#pragma comment (lib, "shell32.lib")
#endif

void OpenUrl (const GS::UniString& url)
{
    if (!url.BeginsWith ("https://") && !url.BeginsWith ("http://")) {
        return;
    }
#if defined (WINDOWS)
    const auto urlUStr = url.ToUStr ();   // owns the UTF-16 string until ShellExecuteW returns
    ShellExecuteW (nullptr, L"open", reinterpret_cast<LPCWSTR> (urlUStr.Get ()), nullptr, nullptr, SW_SHOWNORMAL);
#else
    try {
        GS::Array<GS::UniString> arguments;
        arguments.Push (url);
        GS::Process::Create ("/usr/bin/open", arguments, GS::Process::CreateNoWindow);
    } catch (...) {
    }
#endif
}

GS::UniString EncodeUrlComponent (const GS::UniString& text)
{
    static const char* HexDigits = "0123456789ABCDEF";
    const auto utf8 = text.ToCStr (CC_UTF8);
    GS::UniString encoded;
    for (const char* p = utf8.Get (); *p != '\0'; ++p) {
        const unsigned char c = static_cast<unsigned char> (*p);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~') {
            encoded.Append (GS::UniChar (static_cast<char> (c)));
        } else {
            encoded.Append (GS::UniChar ('%'));
            encoded.Append (GS::UniChar (HexDigits[c >> 4]));
            encoded.Append (GS::UniChar (HexDigits[c & 0x0F]));
        }
    }
    return encoded;
}
