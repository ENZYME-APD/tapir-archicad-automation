using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class DeleteElementSetsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteElementSets";

        public DeleteElementSetsComponent()
            : base(
                "DeleteElementSets",
                "Delete the given element sets. The elements of a set are not deleted. Only element sets " +
                "created by the Tapir Add-On can be deleted; sets created by other add-ons are reported as failed.",
                GroupNames.ElementGrouping)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementSetGuids",
                "Identifiers of the element sets to delete, as CreateElementSets or GetElementSetsOfElements returns them.");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InBoolean(
                "DryRun",
                "If true, nothing is deleted: ErrorMessages says what the deletion would do for each element " +
                "set, from the checks made before deleting (the element set exists and was created by the " +
                "Tapir Add-On). Default is false.",
                false);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message for each element set (empty when it was deleted, or with DryRun when it would " +
                "be deleted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> setWrappers))
            {
                return;
            }

            var elementSets = new JArray();
            foreach (var wrapper in setWrappers)
            {
                var id = GuidObject<ElementSetGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid element set identifier in the ElementSetGuids input.");
                    return;
                }
                elementSets.Add(
                    new JObject
                    {
                        ["elementSetId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            var parameters = new JObject { ["elementSets"] = elementSets };

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
            Properties.Resources.DeleteElementSets;

        public override Guid ComponentGuid =>
            new Guid("02dfd8bf-776b-46fd-b88e-72ff230bee69");
    }
}
