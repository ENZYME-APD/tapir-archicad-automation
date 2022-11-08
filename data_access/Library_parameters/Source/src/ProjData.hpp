#if !defined (ADDITIONALJSONCOMMANDS_HPP)
#define ADDITIONALJSONCOMMANDS_HPP

#pragma once

#include	"APIEnvir.h"
#include	"ACAPinc.h"
#include    "CommandBase.hpp"


class ProjDataCommand : public CommandBase {
public:
	virtual GS::String							GetName() const override;
	//virtual GS::String							GetNamespace() const override;
	virtual GS::Optional<GS::UniString>			GetSchemaDefinitions() const override;
	virtual GS::Optional<GS::UniString>			GetInputParametersSchema() const override;
	virtual GS::Optional<GS::UniString>			GetResponseSchema() const override;
	virtual GS::ObjectState						Execute(const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
#endif