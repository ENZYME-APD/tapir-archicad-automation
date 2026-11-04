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
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message for each element set (empty when it was deleted).");
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

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementSets"] = elementSets },
                ToAddOn,
                da);
        }

        // Shares the icon of CreateElementSets.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateElementSets;

        public override Guid ComponentGuid =>
            new Guid("02dfd8bf-776b-46fd-b88e-72ff230bee69");
    }
}
