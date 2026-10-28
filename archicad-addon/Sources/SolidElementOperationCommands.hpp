#pragma once

#include "CommandBase.hpp"

class CreateSolidElementLinksCommand : public CommandBase
{
public:
    CreateSolidElementLinksCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetRawResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class RemoveSolidElementLinksCommand : public CommandBase
{
public:
    RemoveSolidElementLinksCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetRawResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class TrimElementsCommand : public CommandBase
{
public:
    TrimElementsCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetRawResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class RemoveElementTrimsCommand : public CommandBase
{
public:
    RemoveElementTrimsCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetRawResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetElementTrimsCommand : public CommandBase
{
public:
    GetElementTrimsCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetRawResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetSolidElementLinksCommand : public CommandBase
{
public:
    GetSolidElementLinksCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetRawResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class MergeElementsCommand : public CommandBase
{
public:
    MergeElementsCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetRawResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class UnmergeElementsCommand : public CommandBase
{
public:
    UnmergeElementsCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetRawResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetMergedElementsCommand : public CommandBase
{
public:
    GetMergedElementsCommand ();
    virtual GS::String                          GetName () const override;
    virtual GS::Optional<GS::UniString>         GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString>         GetRawResponseSchema () const override;
    virtual GS::ObjectState                     Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
