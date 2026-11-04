using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.MEP;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPPhysicalSystemsOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPPhysicalSystemsOfElements";

        public GetMEPPhysicalSystemsOfElementsComponent()
            : base(
                "GetMEPPhysicalSystemsOfElements",
                "Returns the physical systems each of the given MEP elements is part of. " +
                "Available from Archicad 29.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the MEP elements.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "PhysicalSystemGuids",
                "The physical systems each element is part of (one branch per element; empty for an element of " +
                "no physical system).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var physicalSystems = new DataTree<object>();
            var items = JsonOutputHelp.Items(response, "physicalSystemsOfElements");
            for (var i = 0; i < items.Count; i++)
            {
                var path = new GH_Path(i);
                physicalSystems.EnsurePath(path);
                if (items[i]["physicalSystemIds"] is JArray ids)
                {
                    foreach (var id in ids)
                    {
                        var guid = JsonOutputHelp.GuidOf(id);
                        physicalSystems.Add(
                            guid == null ? null : new MEPPhysicalSystemGuid { Guid = guid },
                            path);
                    }
                }
            }

            da.SetDataTree(0, physicalSystems);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMEPPhysicalSystems;

        public override Guid ComponentGuid =>
            new Guid("c47b9d9b-25e1-424c-b8e9-ae4c20fe4132");
    }
}
