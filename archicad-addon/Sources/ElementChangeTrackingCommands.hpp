#pragma once

#include "CommandBase.hpp"

// Element change tracking on top of the element difference generator of the Archicad API
// (ACAPI_DifferenceGenerator_GetState / ACAPI_DifferenceGenerator_GenerateDifference,
// APIdefs_ElementDifferenceGenerator.h). CaptureElementState has the generator write a state to a
// file in the temporary folder and keeps it under a stateId; GetElementChangesSince compares a
// stored state with another stored state or with the current project. The states (and their
// files) are dropped by ReleaseElementState, when the project is closed and when Archicad quits.

class CaptureElementStateCommand : public CommandBase
{
public:
    CaptureElementStateCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetElementChangesSinceCommand : public CommandBase
{
public:
    GetElementChangesSinceCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class ReleaseElementStateCommand : public CommandBase
{
public:
    ReleaseElementStateCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

// Called from AddOnMain.cpp: Initialize installs the project event handler that drops every stored
// state when the project is closed or another one is opened; FreeData drops them when the add-on
// unloads, so the state files never outlive the Archicad session.
GSErrCode InstallElementStateProjectEventHandler ();
void      ReleaseAllElementStates ();
