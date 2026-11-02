using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetStructureDisplayComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetStructureDisplay";

        public GetStructureDisplayComponent()
            : base(
                "GetStructureDisplay",
                "Gets the Partial Structure Display (EntireStructure, CoreOnly, WithoutFinishes or StructureOnly) " +
                "of the current window, and whether it is the 3D window. This is the setting of the window itself, " +
                "not the one stored in a saved view (see GetViewSettings). Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "StructureDisplay",
                "The Partial Structure Display mode of the current window: EntireStructure, CoreOnly, " +
                "WithoutFinishes or StructureOnly.");

            OutBoolean(
                "Is3D",
                "True when the setting belongs to the 3D window.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response["structureDisplay"]?.ToString());

            da.SetData(
                1,
                response.Value<bool?>("is3D") ?? false);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetStructureDisplay;

        public override Guid ComponentGuid =>
            new Guid("2e4f5f7d-d94a-448b-b6b8-22ae9f9c0a1d");
    }
}
