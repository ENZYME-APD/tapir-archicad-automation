using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetAutoIntersectComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAutoIntersect";

        public GetAutoIntersectComponent()
            : base(
                "GetAutoIntersect",
                "Gets whether Auto-Intersection (Options > Auto-Intersection) is on. It changes what the wall and " +
                "beam creation commands produce: with it on, the new elements are joined at their intersections.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "IsOn",
                "True if Auto-Intersection is currently on.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            da.SetData(
                0,
                response.Value<bool?>("isOn"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAutoIntersect;

        public override Guid ComponentGuid =>
            new Guid("499a218e-f913-4c92-9c7d-71d394d6bb51");
    }
}
