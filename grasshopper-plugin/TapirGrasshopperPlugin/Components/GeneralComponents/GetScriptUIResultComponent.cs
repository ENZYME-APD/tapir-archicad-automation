using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetScriptUIResultComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetScriptUIResult";

        public GetScriptUIResultComponent()
            : base(
                "GetScriptUIResult",
                "Retrieves and clears the result last submitted from a Script UI window's page " +
                "(via window.ACAPI.SubmitResult), if any, and reports whether the window is still open, " +
                "so a waiting definition can stop when the user closes it. Each refresh clears the result. " +
                "The window is picked by UiId or ScriptId; without them the first window is used.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddIntegerParameter(
                "UiId",
                "UiId",
                ("The Script UI window to read (1 to 8), as output by ShowScriptUI. Takes precedence over ScriptId. " +
                 "Optional; without UiId and ScriptId the first window is read.")
                    .WithTypeName("integerItem"),
                GH_ParamAccess.item);

            InText(
                "ScriptId",
                "The ScriptId given to ShowScriptUI, to read that definition's own window. Optional.");

            SetOptionality(0);
            SetOptionality(1);
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
                "True while the Script UI window is open.");

            OutInteger(
                "UiId",
                "The Script UI window the result was read from (1 to 8). Empty when ScriptId was given " +
                "and that definition has no window.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGet(
                    0,
                    out int uiId))
            {
                parameters["uiId"] = uiId;
            }

            if (da.TryGet(
                    1,
                    out string scriptId) &&
                !string.IsNullOrEmpty(scriptId))
            {
                parameters["scriptId"] = scriptId;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
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

            da.SetData(
                3,
                (int?)response["uiId"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetScriptUIResult;

        public override Guid ComponentGuid =>
            new Guid("63801032-3a6e-41e3-85c7-f18e02712203");
    }
}
