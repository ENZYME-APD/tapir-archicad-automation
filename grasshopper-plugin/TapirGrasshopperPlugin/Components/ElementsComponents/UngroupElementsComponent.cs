using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class UngroupElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "UngroupElements";

        public UngroupElementsComponent()
            : base(
                "UngroupElements",
                "Ungroup the given groups one level, like Edit > Grouping > Ungroup: " +
                "each group is dissolved, the elements and subgroups it directly contains stay.",
                GroupNames.ElementGrouping)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "GroupGuids",
                "Identifiers of the groups to ungroup.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each group (empty when it was ungrouped successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> groupWrappers))
            {
                return;
            }

            var groups = new JArray();
            foreach (var wrapper in groupWrappers)
            {
                var id = GuidObject<GroupGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid group identifier in the GroupGuids input.");
                    return;
                }
                groups.Add(
                    new JObject
                    {
                        ["groupId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            var parameters = new JObject { ["groups"] = groups };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetDataList(
                0,
                ExecutionResultsResponse.ErrorMessages(response));
        }

        // IconSource/UngroupElements.svg is the icon's source; until it is rendered
        // to Resources/UngroupElements.png and listed in Resources.resx, the
        // component shows the CreateGroups bitmap.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateGroups;

        public override Guid ComponentGuid =>
            new Guid("7c1e4b2a-9d3f-4e8a-b5c6-2f0a7d9e1b43");
    }
}
