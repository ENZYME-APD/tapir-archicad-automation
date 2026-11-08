#pragma once

#include "CommandBase.hpp"

// Version-independent wrappers of the library part creation calls, for a library part under creation.
GSErrCode NewLibPartSection (const API_LibPartSection* section);
GSErrCode WriteLibPartSection (Int32 nByte, GSConstPtr data);
GSErrCode EndLibPartSection ();
GSErrCode SetLibPartDetails (const API_LibPart* libPart, GSHandle paramHdl, const API_LibPartDetails* details);

class CreateLibraryPartFromElementsCommand : public CommandBase
{
public:
    CreateLibraryPartFromElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class InterpretGDLScript3DCommand : public CommandBase
{
public:
    InterpretGDLScript3DCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetGDLUserGlobalsCommand : public CommandBase
{
public:
    GetGDLUserGlobalsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
