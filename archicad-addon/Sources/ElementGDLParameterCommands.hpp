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
    GSErrCode SetOneGDLParameter (const GS::ObjectState& parameter, const API_Guid& elemGuid, API_ChangeParamType& changeParam, const GS::HashTable<GS::String, API_AddParID>& gdlParametersTypeDictionary, GS::UniString& errMessage) const;
};
