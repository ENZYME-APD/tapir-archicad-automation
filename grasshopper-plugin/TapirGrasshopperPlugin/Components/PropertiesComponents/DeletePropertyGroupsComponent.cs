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
    public class DeletePropertyGroupsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeletePropertyGroups";

        public DeletePropertyGroupsComponent()
            : base(
                "DeletePropertyGroups",
                "Delete the given Custom Property Groups.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGroupGuids",
                "Identifiers of the property groups to delete.");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is deleted: ErrorMessages says what the deletion would do for each " +
                "property group, from the check made before deleting (the property group exists). Archicad " +
                "can still refuse to delete a group that exists, for example a built-in group. Default is " +
                "false.",
                false);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each property group (empty when it was deleted, or with DryRun when it " +
                "would be deleted).");
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

            var input = new DeletePropertyGroupsParameters
            {
                PropertyGroupIds = new List<PropertyGroupGuidWrapper>()
            };

            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<PropertyGroupGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid property group identifier in the PropertyGroupGuids input.");
                    return;
                }
                input.PropertyGroupIds.Add(
                    new PropertyGroupGuidWrapper { PropertyGroupId = id });
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
            Properties.Resources.DeletePropertyGroups;

        public override Guid ComponentGuid =>
            new Guid("16b0cce6-3ce8-4a68-a777-fde8e6744e57");
    }
}
