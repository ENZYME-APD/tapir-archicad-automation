using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetStoryControlPrimitivesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetStoryControlPrimitives";

        public GetStoryControlPrimitivesComponent()
            : base(
                "GetStoryControlPrimitives",
                "Returns the 2D drawing primitives of the story levels (story lines and their markers) drawn " +
                "in the given section, elevation or interior elevation database, in the same format as " +
                "GetDrawingPrimitivesOfElements. The database identifier can be obtained with " +
                "GetDatabaseIdFromNavigatorItemId.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "DatabaseGuid",
                "The identifier of a section, elevation or interior elevation database.");
        }

        protected override void AddOutputs()
        {
            DrawingPrimitivesComponentHelp.AddOutputs(
                this,
                "");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out DatabaseGuidObject databaseId))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["databaseId"] = JObject.FromObject(databaseId) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var output = new DrawingPrimitivesOutput();
            output.AddPrimitives(
                response["primitives"],
                new GH_Path(0));

            DrawingPrimitivesComponentHelp.SetOutputs(
                da,
                output);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetStoryControlPrimitives;

        public override Guid ComponentGuid =>
            new Guid("2fb03b59-838a-4bbc-a7de-cd5a06a3d8fb");
    }
}
