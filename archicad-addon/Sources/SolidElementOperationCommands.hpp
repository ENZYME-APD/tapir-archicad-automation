#pragma once

#include "CommandBase.hpp"

class CreateSolidElementLinksCommand : public CommandBase
{
public:
    CreateSolidElementLinksCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class RemoveSolidElementLinksCommand : public CommandBase
{
public:
    RemoveSolidElementLinksCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetSolidElementLinksCommand : public CommandBase
{
public:
    GetSolidElementLinksCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
