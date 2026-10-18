#pragma once

#include "CommandBase.hpp"

// Graphic override rules and combinations. The commands need Archicad 27 or later, where
// override rules live in rule groups.

class CreateGraphicalOverrideRuleCommand : public CommandBase
{
public:
    CreateGraphicalOverrideRuleCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetGraphicalOverrideRulesCommand : public CommandBase
{
public:
    GetGraphicalOverrideRulesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class CreateGraphicalOverrideCombinationCommand : public CommandBase
{
public:
    CreateGraphicalOverrideCombinationCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
