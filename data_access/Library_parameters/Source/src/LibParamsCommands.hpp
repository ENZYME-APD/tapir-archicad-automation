#if !defined (LIBCOMMANDS_HPP)
#define LIBCOMMANNDS_HPP

#pragma once

#include	"APIEnvir.h"
#include	"ACAPinc.h"
#include    "CommandBase.hpp"

class LibCommand : public CommandBase {
public:
	virtual GS::String							GetName() const override;
	//virtual GS::String							GetNamespace() const override;
	virtual GS::Optional<GS::UniString>			GetSchemaDefinitions() const override;
	virtual GS::Optional<GS::UniString>			GetInputParametersSchema() const override;
	virtual GS::Optional<GS::UniString>			GetResponseSchema() const override;
	virtual GS::ObjectState						Execute(const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
#endif