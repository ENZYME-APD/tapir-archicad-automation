#pragma once

#include "CommandBase.hpp"

// The Analytical Model API and the MEP API are libraries of their own (AnalyticalModelAPI.dll and MEPAPI.dll on
// Windows, AnalyticalModelAPI.framework and MEPAPI.framework on macOS), and not every Archicad can load them (#1688).
// So that Tapir loads without them, Tools/CMakeCommon.cmake delay-loads them on Windows and links them weakly on
// macOS. A call into one of them that is missing crashes Archicad, so every code path that calls the Analytical
// Model API or the MEP API checks these first. The commands stay registered and answer with the responses below.

// True when the Analytical Model API can be called. Archicad 25 has no such library: there the functions are part
// of the Archicad API itself, so it is always true.
bool IsAnalyticalModelAPIAvailable ();

// True when the MEP API can be called. Always false before Archicad 27, which has no MEP API.
bool IsMEPAPIAvailable ();

GS::ObjectState CreateAnalyticalModelAPINotAvailableResponse ();
GS::ObjectState CreateMEPAPINotAvailableResponse ();

// The element type IDs these libraries export as data. Data cannot be delay-loaded, so the code reads them through
// these instead of the variables of the DevKit headers. Call them only when the library is available.
#ifdef ServerMainVers_2600
const API_Guid& GetAnalyticalSupportID ();
const API_Guid& GetAnalyticalLinkID ();
const API_Guid& GetAnalyticalPointLoadID ();
const API_Guid& GetAnalyticalEdgeLoadID ();
const API_Guid& GetAnalyticalSurfaceLoadID ();
#endif

#ifdef ServerMainVers_3000
const API_Guid& GetMEPElectricalPanelID ();
const API_Guid& GetMEPElectricalDeviceID ();
#endif
