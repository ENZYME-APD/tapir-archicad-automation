#pragma once

#include "CommandBase.hpp"

class GetGDLParametersOfElementsCommand : public CommandBase
{
public:
    GetGDLParametersOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class SetGDLParametersOfElementsCommand : public CommandBase
{
public:
    SetGDLParametersOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
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
        GS::Array<ArrayParameterChange>& pendingArrayChanges,
        GS::UniString& errMessage);

    static bool ParseArrayParameterValue (
        const GS::ObjectState& parameter,
        API_AddParID typeID,
        ArrayParameterChange& change);

    static GSErrCode ApplyArrayParameterChanges (
        const API_GetParamsType& getParams,
        const GS::Array<ArrayParameterChange>& changes,
        const API_Guid& elemGuid,
        GS::UniString& errMessage);
};
