using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartParameterValuesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartParameterValues";

        public GetLibraryPartParameterValuesComponent()
            : base(
                "GetLibraryPartParameterValues",
                "Gets the parameters of loaded library parts, given by their file name, with their default values " +
                "and descriptions, in the format of GetGDLParametersOfElements: the read counterpart of " +
                "SetLibraryPartParameters. Unlike GetGDLParametersOfLibraryParts, which finds a part by its name " +
                "and also returns the index, flags and value descriptions of each parameter, it reads through the " +
                "Library Manager. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each loaded library part file with its extension, as the fileName of " +
                "GetAvailableLibraryParts (e.g. 'My Door.gsm').");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "The name of each library part read (null when it could not be read).");

            OutTextTree(
                "ParameterNames",
                "The name of each GDL parameter (one branch per library part). Separators, titles and dictionary " +
                "parameters have no value and are left out.");

            OutTextTree(
                "ParameterDescriptions",
                "The description of each GDL parameter, the text shown for it in the settings dialog (null when " +
                "Archicad gives none; one branch per library part).");

            OutTexts(
                "Parameters",
                "The GDL parameters of each library part with their default values, as a JSON array of objects " +
                "with name, type, value and for arrays dimension1 and dimension2 (angles in radians; the " +
                "descriptions are in ParameterDescriptions). SetLibraryPartParameters takes it back.");

            OutErrorMessages(
                "Error message of each library part (empty when its parameters were read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names) ||
                names.Count == 0)
            {
                return;
            }

            var libraryParts = new JArray();
            foreach (var name in names)
            {
                libraryParts.Add(new JObject { ["name"] = name });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["libraryParts"] = libraryParts },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var outNames = new List<object>();
            var parameterNames = new DataTree<string>();
            var parameterDescriptions = new DataTree<string>();
            var parameters = new List<string>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "libraryParts");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                parameterNames.EnsurePath(path);
                parameterDescriptions.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    outNames.Add(null);
                    parameters.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                outNames.Add(JsonOutputHelp.Scalar(item, "name"));
                if (item["parameters"] is JArray list)
                {
                    // SetLibraryPartParameters refuses keys it does not know,
                    // so the descriptions go to their own output.
                    var values = new JArray();
                    foreach (var parameter in list)
                    {
                        parameterNames.Add(parameter["name"]?.ToString(), path);
                        parameterDescriptions.Add(parameter["description"]?.ToString(), path);

                        var value = parameter.DeepClone();
                        if (value is JObject valueObject)
                        {
                            valueObject.Remove("description");
                        }
                        values.Add(value);
                    }
                    parameters.Add(values.ToString(Formatting.None));
                }
                else
                {
                    parameters.Add(null);
                }
                errors.Add("");
            }

            da.SetDataList(0, outNames);
            da.SetDataTree(1, parameterNames);
            da.SetDataTree(2, parameterDescriptions);
            da.SetDataList(3, parameters);
            da.SetDataList(4, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLibraryPartParameterValues;

        public override Guid ComponentGuid =>
            new Guid("f4de966b-4231-4baf-a42e-f411a0f93934");
    }
}
