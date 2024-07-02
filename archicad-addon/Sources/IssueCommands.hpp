#pragma once

#include "CommandBase.hpp"

class CreateIssueCommand : public CommandBase
{
public:
    CreateIssueCommand ();
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class DeleteIssueCommand : public CommandBase
{
public:
    DeleteIssueCommand ();
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetIssueListCommand : public CommandBase
{
public:
    GetIssueListCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetCommentsCommand : public CommandBase
{
public:
    GetCommentsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class AddCommentCommand : public CommandBase
{
public:
    AddCommentCommand ();
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class AttachElementsCommand : public CommandBase
{
public:
    AttachElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class DetachElementsCommand : public CommandBase
{
public:
    DetachElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetAttachedElementsCommand : public CommandBase
{
public:
    GetAttachedElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class ExportToBCFCommand : public CommandBase
{
public:
    ExportToBCFCommand ();
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class ImportFromBCFCommand : public CommandBase
{
public:
    ImportFromBCFCommand ();
    virtual GS::String GetName () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};