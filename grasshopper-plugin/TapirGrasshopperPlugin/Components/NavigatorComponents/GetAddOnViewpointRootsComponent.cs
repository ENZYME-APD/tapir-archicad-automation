using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetAddOnViewpointRootsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAddOnViewpointRoots";

        public GetAddOnViewpointRootsComponent()
            : base(
                "GetAddOnViewpointRoots",
                "Returns the roots of the add-on viewpoints: the Project Map items created through the Archicad " +
                "API by an add-on, as Archicad lists them to Tapir. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ViewpointIds",
                "The identifier of each add-on viewpoint root.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    JHelp.Deserialize<AddOnViewpointsObject>,
                    out AddOnViewpointsObject response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.ViewpointIds?.Select(x => x.Id) ??
                Enumerable.Empty<AddOnViewpointGuid>());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAddOnViewpointRoots;

        public override Guid ComponentGuid =>
            new Guid("28f01563-4a06-49cf-915f-5ca994908730");
    }
}
