using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class DeletePropertyDefinitionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeletePropertyDefinitions";

        public DeletePropertyDefinitionsComponent()
            : base(
                "DeletePropertyDefinitions",
                "Delete the given Custom Property Definitions.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "Identifiers of the property definitions to delete.");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is deleted: ErrorMessages says what the deletion would do for each " +
                "property definition, from the check made before deleting (the property exists). Archicad " +
                "can still refuse to delete a property that exists, for example a built-in property. Default " +
                "is false.",
                false);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each property definition (empty when it was deleted, or with DryRun when " +
                "it would be deleted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            var input = new DeletePropertyDefinitionsParameters
            {
                PropertyIds = new List<PropertyGuidWrapper>()
            };

            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<PropertyGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid property identifier in the PropertyGuids input.");
                    return;
                }
                input.PropertyIds.Add(
                    new PropertyGuidWrapper { PropertyId = id });
            }

            var parameters = JObject.FromObject(input);

            // Only sent when it differs from the command's default.
            if (da.GetOptional(1, false))
            {
                parameters["dryRun"] = true;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeletePropertyDefinitions;

        public override Guid ComponentGuid =>
            new Guid("b6ca0582-df75-4a1d-9222-94ed7fa597e1");
    }
}
