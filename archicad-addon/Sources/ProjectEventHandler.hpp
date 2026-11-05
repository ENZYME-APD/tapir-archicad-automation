#pragma once

#include "CommandBase.hpp"

// Archicad keeps one project event handler per add-on: a second
// ACAPI_ProjectOperation_CatchProjectEvent call replaces the first one. So every part of the
// add-on that needs project events is served by this one handler, installed once in Initialize:
// the element states of CaptureElementState (released when the project closes) and the
// notification clients of SetElementNotificationClient (window, floor and project events).
GSErrCode InstallProjectEventHandler ();
