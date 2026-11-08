using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetAddOnViewpointChildrenComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAddOnViewpointChildren";

        public GetAddOnViewpointChildrenComponent()
            : base(
                "GetAddOnViewpointChildren",
                "Returns the direct children of an add-on viewpoint root or group. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "ViewpointId",
                "The add-on viewpoint root or group to list the children of.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ViewpointIds",
                "The identifier of each direct child.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out AddOnViewpointGuidWrapper viewpoint))
            {
                this.AddError(
                    "ViewpointId is not a valid add-on viewpoint identifier.");
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    viewpoint,
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
            Properties.Resources.GetAddOnViewpointChildren;

        public override Guid ComponentGuid =>
            new Guid("db0d6e09-9084-4a6c-b2ee-2347cb0b7a5c");
    }
}
