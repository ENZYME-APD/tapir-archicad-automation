#pragma once

#include "CommandBase.hpp"

class GetGDLParametersOfElementsCommand : public CommandBase
{
public:
    GetGDLParametersOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class SetGDLParametersOfElementsCommand : public CommandBase
{
public:
    SetGDLParametersOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;

private:
    struct ArrayParameterChange {
        GS::String name;
        API_AddParID typeID = APIParT_Integer;
        Int32 dim1 = 0;
        Int32 dim2 = 0;
        GS::Array<double> numberValues;
        GS::Array<GS::UniString> stringValues;
    };

    static GSErrCode SetOneGDLParameter (
        const GS::ObjectState& parameter,
        const API_Guid& elemGuid,
        const API_GetParamsType& getParams,
        const GS::HashTable<GS::String, API_AddParID>& gdlParametersTypeDictionary,
        const GS::HashTable<short, GS::String>& gdlParametersIndexNameDictionary,
        const GS::Optional<double>& xRatioPerA,
        const GS::Optional<double>& yRatioPerB,
        GS::Array<ArrayParameterChange>& pendingArrayChanges,
        GS::UniString& errMessage);

    static bool ParseArrayParameterValue (
        const GS::ObjectState& parameter,
        API_AddParID typeID,
        ArrayParameterChange& change);

    static GSErrCode ApplyArrayParameterChanges (
        API_GetParamsType& getParams,
        const GS::Array<ArrayParameterChange>& changes,
        const API_Guid& elemGuid,
        GS::UniString& errMessage);
};

// The marker (the stamp drawn next to a window or a door on the floor plan) is a Main
// Marker sub-element with a library part and GDL parameters of its own, separate from
// the opening's. Get/SetGDLParametersOfElements reach the opening only, so these two
// commands read and write the marker's parameters.
class GetMarkerParametersOfElementsCommand : public CommandBase
{
public:
    GetMarkerParametersOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class SetMarkerParametersOfElementsCommand : public CommandBase
{
public:
    SetMarkerParametersOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
