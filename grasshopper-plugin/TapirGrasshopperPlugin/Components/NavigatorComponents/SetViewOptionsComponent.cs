using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetViewOptionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetViewOptions";

        public SetViewOptionsComponent()
            : base(
                "SetViewOptions",
                "Switches the current plan window to a model view option combination by index, and/or changes the " +
                "given model view options and on-screen view options. The fields are named as in the outputs of " +
                "GetViewOptions; the fields not given keep their current value. With an index, the model view " +
                "options stored in the combination apply; without one, the window uses Custom model view options.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "ModelViewOptionIndex",
                "Switches the window to the model view option combination with this index, as FindModelViewOption " +
                "returns it. Archicad then applies the model view options stored in the combination, not the ones " +
                "in ModelViewOptions_JSON. Find the index by name right before switching: Archicad may renumber the " +
                "combinations when the view options of a window change. Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "ModelViewOptions_JSON",
                "The model view options to change as a JSON object, named as in the ModelViewOptions_JSON output of " +
                "GetViewOptions. Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "OnScreenViewOptions_JSON",
                "The on-screen view options to change as a JSON object, named as in the OnScreenViewOptions_JSON " +
                "output of GetViewOptions. Optional.");
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Success",
                "True when the view options were changed.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            NavigatorComponentHelp.CopyOptional<int>(da, 0, parameters, "modelViewOptionIndex");

            var viewOptions = new JObject();
            if (!TryCopyJsonObject(da, 1, "ModelViewOptions_JSON", viewOptions, "modelViewOptions") ||
                !TryCopyJsonObject(da, 2, "OnScreenViewOptions_JSON", viewOptions, "onScreenViewOptions"))
            {
                return;
            }

            if (viewOptions.Count > 0)
            {
                parameters["viewOptions"] = viewOptions;
            }

            if (parameters.Count == 0)
            {
                this.AddError("Give ModelViewOptionIndex, a view options JSON or both.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, true);
        }

        private bool TryCopyJsonObject(
            IGH_DataAccess da,
            int index,
            string inputName,
            JObject target,
            string key)
        {
            var text = "";
            if (!da.GetData(index, ref text) ||
                string.IsNullOrWhiteSpace(text))
            {
                return true;
            }

            try
            {
                target[key] = JObject.Parse(text);
                return true;
            }
            catch (JsonReaderException ex)
            {
                this.AddError($"The input {inputName} is not a valid JSON object: {ex.Message}");
                return false;
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetViewOptions;

        public override Guid ComponentGuid =>
            new Guid("d6c37f12-5c24-49f7-be2c-dea95e128115");
    }
}
