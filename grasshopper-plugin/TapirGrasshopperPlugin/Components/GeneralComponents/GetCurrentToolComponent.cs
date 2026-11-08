using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetCurrentToolComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCurrentTool";

        public GetCurrentToolComponent()
            : base(
                "GetCurrentTool",
                "Gets the tool active in the Toolbox: its element type, its variation and, for the tool of an " +
                "add-on, its class id. Connect the outputs to SetCurrentTool to activate the same tool again. " +
                "All outputs are empty when the Arrow or the Marquee is active.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "ElementType",
                "The element type of the active tool (e.g. Wall, Door, Object). Unknown for the tool of an " +
                "add-on, which ClassId identifies. Empty for the Arrow and the Marquee.");

            OutText(
                "Variation",
                "The four-character subtype of the active tool (e.g. SKYL, CRWD, or the signature of an " +
                "add-on's library part subtype). Empty for the generic tool of the element type.");

            OutText(
                "ClassId",
                "The class id (a guid) of the tool of an add-on. Empty for the other tools. Archicad 26 and later.");
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
                response?["elementType"]?.ToString());

            da.SetData(
                1,
                response?["variation"]?.ToString());

            da.SetData(
                2,
                response?["classId"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCurrentWindowType;

        public override Guid ComponentGuid =>
            new Guid("4c0f6b8e-2d6a-4f3e-9a51-7e2b1c8d3f64");
    }
}
