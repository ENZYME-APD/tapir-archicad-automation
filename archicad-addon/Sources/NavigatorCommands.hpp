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

class GetModelViewOptionsCommand : public CommandBase
{
public:
    GetModelViewOptionsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetViewSettingsCommand : public CommandBase
{
public:
    GetViewSettingsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class SetViewSettingsCommand : public CommandBase
{
public:
    SetViewSettingsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetView2DTransformationsCommand : public CommandBase
{
public:
    GetView2DTransformationsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};