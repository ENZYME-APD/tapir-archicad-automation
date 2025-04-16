#pragma once

#include "CommandBase.hpp"

class PublishPublisherSetCommand : public CommandBase
{
public:
    PublishPublisherSetCommand();
    virtual GS::String GetName() const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema() const override;
    virtual GS::ObjectState Execute(const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class UpdateDrawingsCommand : public CommandBase
{
public:
    UpdateDrawingsCommand();
    virtual GS::String GetName() const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema() const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema() const override;
    virtual GS::ObjectState Execute(const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetDatabaseIdFromNavigatorItemIdCommand : public CommandBase
{
public:
    GetDatabaseIdFromNavigatorItemIdCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetNavigatorViewsCommand : public CommandBase
{
public:
    GetNavigatorViewsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class SetNavigatorViewsCommand : public CommandBase
{
public:
    SetNavigatorViewsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};