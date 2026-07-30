#pragma once

#include "CommandBase.hpp"

class CreateGroupsCommand : public CommandBase
{
public:
    CreateGroupsCommand();
    virtual GS::String GetName() const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema() const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema() const override;
    virtual GS::ObjectState Execute(const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetGroupsOfElementsCommand : public CommandBase
{
public:
    GetGroupsOfElementsCommand();
    virtual GS::String GetName() const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema() const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema() const override;
    virtual GS::ObjectState Execute(const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetElementsOfGroupsCommand : public CommandBase
{
public:
    GetElementsOfGroupsCommand();
    virtual GS::String GetName() const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema() const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema() const override;
    virtual GS::ObjectState Execute(const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetSuspendGroupsModeCommand : public CommandBase
{
public:
    GetSuspendGroupsModeCommand();
    virtual GS::String GetName() const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema() const override;
    virtual GS::ObjectState Execute(const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class SetSuspendGroupsModeCommand : public CommandBase
{
public:
    SetSuspendGroupsModeCommand();
    virtual GS::String GetName() const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema() const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema() const override;
    virtual GS::ObjectState Execute(const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};