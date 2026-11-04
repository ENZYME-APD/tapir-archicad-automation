#include "OptionalAPIs.hpp"

#ifdef ServerMainVers_2600

#if defined (WINDOWS)

#include <delayimp.h>

static const char* const AnalyticalModelAPIDll = "AnalyticalModelAPI.dll";
#ifdef ServerMainVers_2700
static const char* const MEPAPIDll = "MEPAPI.dll";
#endif

// A delay-loaded DLL or function that cannot be found is reported with one of these structured exceptions.
static int DelayLoadFailureFilter (DWORD exceptionCode)
{
    if (exceptionCode == static_cast<DWORD> (VcppException (ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)) ||
        exceptionCode == static_cast<DWORD> (VcppException (ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND))) {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

// Loads the delay-loaded DLL and binds every function Tapir imports from it, so no later call into it can fail.
// A function with __try cannot unwind C++ objects, so this one has none.
static HRESULT LoadAllImportsOfDll (const char* dllName)
{
    HRESULT result = E_FAIL;
    __try {
        result = __HrLoadAllImportsForDll (dllName);
    } __except (DelayLoadFailureFilter (GetExceptionCode ())) {
        result = E_FAIL;
    }
    return result;
}

static bool IsDllAvailable (const char* dllName)
{
    const HRESULT result = LoadAllImportsOfDll (dllName);
    if (SUCCEEDED (result)) {
        return true;
    }
    // Without an exception this error means the DLL is not delay-loaded at all. Then it is linked normally,
    // and Archicad could only load Tapir because the DLL is loaded.
    if (result == HRESULT_FROM_WIN32 (ERROR_MOD_NOT_FOUND)) {
        return GetModuleHandleA (dllName) != nullptr;
    }
    return false;
}

// The value of a variable the DLL exports. GetProcAddress gives the address of an exported variable.
static const API_Guid& GetExportedGuid (const char* dllName, const char* exportName)
{
    const HMODULE module = GetModuleHandleA (dllName);
    const FARPROC address = (module != nullptr) ? GetProcAddress (module, exportName) : nullptr;
    if (address == nullptr) {
        return APINULLGuid;
    }
    return *reinterpret_cast<const API_Guid*> (address);
}

#else

#include "ACAPI_Analytical.hpp"
#ifdef ServerMainVers_2700
#include "ACAPI/MEPAdapter.hpp"
#endif

// The frameworks are linked weakly: when one is missing, dyld sets the addresses of its symbols to null instead
// of refusing to load Tapir. The address is read back through a volatile, so the compiler cannot treat it as the
// address of an existing object and fold the test to true.
static bool IsWeakSymbolBound (const void* address)
{
    const void* volatile boundAddress = address;
    return boundAddress != nullptr;
}

#endif

#endif

bool IsAnalyticalModelAPIAvailable ()
{
#if defined (ServerMainVers_2600)
#if defined (WINDOWS)
    static const bool available = IsDllAvailable (AnalyticalModelAPIDll);
#else
    static const bool available = IsWeakSymbolBound (&API_AnalyticalSupportID);
#endif
    return available;
#else
    return true;
#endif
}

bool IsMEPAPIAvailable ()
{
#if defined (ServerMainVers_2700)
#if defined (WINDOWS)
    static const bool available = IsDllAvailable (MEPAPIDll);
#else
    static const bool available = IsWeakSymbolBound (&ACAPI::MEP::VentilationBranchID);
#endif
    return available;
#else
    return false;
#endif
}

GS::ObjectState CreateAnalyticalModelAPINotAvailableResponse ()
{
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command needs the Analytical Model API, which this Archicad does not provide.");
}

GS::ObjectState CreateMEPAPINotAvailableResponse ()
{
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command needs the MEP API, which this Archicad does not provide.");
}

#ifdef ServerMainVers_2600

const API_Guid& GetAnalyticalSupportID ()
{
#if defined (WINDOWS)
    static const API_Guid& id = GetExportedGuid (AnalyticalModelAPIDll, "API_AnalyticalSupportID");
    return id;
#else
    return API_AnalyticalSupportID;
#endif
}

const API_Guid& GetAnalyticalLinkID ()
{
#if defined (WINDOWS)
    static const API_Guid& id = GetExportedGuid (AnalyticalModelAPIDll, "API_AnalyticalLinkID");
    return id;
#else
    return API_AnalyticalLinkID;
#endif
}

const API_Guid& GetAnalyticalPointLoadID ()
{
#if defined (WINDOWS)
    static const API_Guid& id = GetExportedGuid (AnalyticalModelAPIDll, "API_AnalyticalPointLoadID");
    return id;
#else
    return API_AnalyticalPointLoadID;
#endif
}

const API_Guid& GetAnalyticalEdgeLoadID ()
{
#if defined (WINDOWS)
    static const API_Guid& id = GetExportedGuid (AnalyticalModelAPIDll, "API_AnalyticalEdgeLoadID");
    return id;
#else
    return API_AnalyticalEdgeLoadID;
#endif
}

const API_Guid& GetAnalyticalSurfaceLoadID ()
{
#if defined (WINDOWS)
    static const API_Guid& id = GetExportedGuid (AnalyticalModelAPIDll, "API_AnalyticalSurfaceLoadID");
    return id;
#else
    return API_AnalyticalSurfaceLoadID;
#endif
}

#endif

#ifdef ServerMainVers_3000

// The decorated names MEPAPI.dll exports the ACAPI::MEP variables with (from the import library of the DevKit).
const API_Guid& GetMEPElectricalPanelID ()
{
#if defined (WINDOWS)
    static const API_Guid& id = GetExportedGuid (MEPAPIDll, "?ElectricalPanelID@MEP@ACAPI@@3UAPI_Guid@@B");
    return id;
#else
    return ACAPI::MEP::ElectricalPanelID;
#endif
}

const API_Guid& GetMEPElectricalDeviceID ()
{
#if defined (WINDOWS)
    static const API_Guid& id = GetExportedGuid (MEPAPIDll, "?ElectricalDeviceID@MEP@ACAPI@@3UAPI_Guid@@B");
    return id;
#else
    return ACAPI::MEP::ElectricalDeviceID;
#endif
}

#endif
