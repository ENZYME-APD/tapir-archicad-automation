using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetActiveGraphicalOverrideCombinationComponent
        : ArchicadExecutorComponent
    {
        public override string CommandName =>
            "SetActiveGraphicalOverrideCombination";

        public SetActiveGraphicalOverrideCombinationComponent()
            : base(
                "SetActiveGraphicalOverrideCombination",
                "Applies a graphic override combination, given by identifier or by name, to the current window of Archicad. " +
                "The saved views are not changed (see SetViewSettings). Available from Archicad 30.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "CombinationGuid",
                "Identifier of the graphic override combination, e.g. from ActiveGraphicalOverrideCombination. Give either this or Name.");

            InText(
                "Name",
                "Name of the graphic override combination, as shown in Document > Graphic Overrides > Graphic Override Combinations. Give either this or CombinationGuid.");

            SetOptionality(
                new[]
                {
                    0,
                    1
                });
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            GH_ObjectWrapper guidWrapper = null;
            da.GetData(
                0,
                ref guidWrapper);

            string name = null;
            da.GetData(
                1,
                ref name);

            string guid = null;
            if (guidWrapper?.Value != null)
            {
                guid = guidWrapper.Value is GH_String ghString
                    ? ghString.Value
                    : guidWrapper.Value.ToString();
                if (!Guid.TryParse(
                        guid,
                        out _))
                {
                    this.AddError("CombinationGuid is not a valid identifier.");
                    return;
                }
            }

            var hasGuid = !string.IsNullOrEmpty(guid);
            var hasName = !string.IsNullOrEmpty(name);
            if (hasGuid == hasName)
            {
                this.AddError("Give exactly one of CombinationGuid and Name.");
                return;
            }

            var parameters = new JObject();
            if (hasGuid)
            {
                parameters["combinationId"] = new JObject { ["guid"] = guid };
            }
            else
            {
                parameters["name"] = name;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(
                0,
                response.Message());
        }

        // Shares the icon of SetViewSettings until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetViewSettings;

        public override Guid ComponentGuid =>
            new Guid("b4e7d2a9-81c3-4f5e-9a06-3d5c8e1f7b42");
    }
}
