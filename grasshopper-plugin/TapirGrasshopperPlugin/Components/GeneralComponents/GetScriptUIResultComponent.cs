using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetScriptUIResultComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetScriptUIResult";

        public GetScriptUIResultComponent()
            : base(
                "GetScriptUIResult",
                "Retrieves and clears the result last submitted from the Script UI palette's page " +
                "(via window.ACAPI.SubmitResult), if any, and reports whether the palette is still open, " +
                "so a waiting definition can stop when the user closes it. Each refresh clears the result.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "HasResult",
                "True if the Script UI page has submitted a result since the last call.");

            OutText(
                "Result",
                "The submitted content (as passed to window.ACAPI.SubmitResult). Empty when HasResult is false.");

            OutBoolean(
                "IsOpen",
                "True while the Script UI palette is open.");
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
                (bool?)response["hasResult"]);

            da.SetData(
                1,
                response["result"]?.ToString());

            da.SetData(
                2,
                (bool?)response["isOpen"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetScriptUIResult;

        public override Guid ComponentGuid =>
            new Guid("63801032-3a6e-41e3-85c7-f18e02712203");
    }
}
