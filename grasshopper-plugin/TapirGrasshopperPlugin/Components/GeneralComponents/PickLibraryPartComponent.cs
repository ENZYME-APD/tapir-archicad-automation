using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class PickLibraryPartComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PickLibraryPart";

        public PickLibraryPartComponent()
            : base(
                "PickLibraryPart",
                "Opens Archicad's Object, Lamp, Window or Door Settings dialog, optionally limited to a library part " +
                "subtype, lets the user pick a library part and set its parameters, and returns the chosen library " +
                "part with its GDL parameters, or that the user cancelled. Places nothing. Archicad waits for the " +
                "user, so it only runs on the Execute button.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ElementType",
                "Whose settings dialog to open: Object, Lamp, Window or Door. Without SubtypeUnID the dialog lists " +
                "the library parts of that tool. Optional; default: Object.");

            InText(
                "SubtypeUnID",
                "Limits the dialog to the library parts of this subtype: the unique ID of the subtype library part, " +
                "its main and revision GUID in the form {main}-{revision}. Optional.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Cancelled",
                "True when the user closed the dialog with Cancel; then the other outputs are empty.");

            OutText(
                "LibPartName",
                "The name of the chosen library part.");

            OutText(
                "OwnUnID",
                "The unique ID (guid) of the chosen library part.");

            OutText(
                "ParentUnID",
                "The unique ID (guid) of the parent of the chosen library part.");

            OutText(
                "SubtypeUnID",
                "The unique ID of the chosen library part's subtype, as Archicad returned it.");

            OutTexts(
                "ParameterNames",
                "The name of each GDL parameter of the chosen library part.");

            OutGenerics(
                "ParameterValues",
                "The value of each GDL parameter, as set in the dialog (arrays as JSON text).");

            OutTexts(
                "Parameters",
                "Each GDL parameter as JSON: name, displayName, index, type, dimension1, dimension2, value, " +
                "valueDescription and, when the library part defines them, itemDescriptions.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            string elementType = null;
            if (da.GetData(0, ref elementType) &&
                !string.IsNullOrEmpty(elementType))
            {
                parameters["elementType"] = elementType;
            }

            string subtypeUnID = null;
            if (da.GetData(1, ref subtypeUnID) &&
                !string.IsNullOrEmpty(subtypeUnID))
            {
                parameters["subtypeUnID"] = subtypeUnID;
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
                (bool?)response["cancelled"]);

            var libPart = response["libPart"];
            da.SetData(
                1,
                libPart?["name"]?.ToString());
            da.SetData(
                2,
                JsonOutputHelp.GuidOf(libPart?["ownUnID"]));
            da.SetData(
                3,
                JsonOutputHelp.GuidOf(libPart?["parentUnID"]));
            da.SetData(
                4,
                response["subtypeUnID"]?.ToString());

            var parameterNames = new List<string>();
            var parameterValues = new List<object>();
            var parameterJsons = new List<string>();
            foreach (var parameter in JsonOutputHelp.Items(
                         response,
                         "parameters"))
            {
                parameterNames.Add(parameter["name"]?.ToString());
                parameterValues.Add(ValueOf(parameter["value"]));
                parameterJsons.Add(parameter.ToString(Formatting.None));
            }

            da.SetDataList(5, parameterNames);
            da.SetDataList(6, parameterValues);
            da.SetDataList(7, parameterJsons);
        }

        // A scalar parameter value as it is; an array value as JSON text.
        private static object ValueOf(
            JToken value)
        {
            if (value == null || value.Type == JTokenType.Null)
            {
                return null;
            }

            if (value is JValue scalar)
            {
                return scalar.Value;
            }

            return value.ToString(Formatting.None);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PickLibraryPart;

        public override Guid ComponentGuid =>
            new Guid("9d75c696-775c-446f-b510-40f5608a20be");
    }
}
