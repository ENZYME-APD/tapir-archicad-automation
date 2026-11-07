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
    public class GetGDLParametersOfLibraryPartsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGDLParametersOfLibraryParts";

        public GetGDLParametersOfLibraryPartsComponent()
            : base(
                "GetGDLParametersOfLibraryParts",
                "Gets the default GDL parameters (name, display name, type, array dimensions, value, flags) of " +
                "library parts by name, read from the library part without placing it or changing any defaults. " +
                "The format is that of GetGDLParametersOfElements, without isLocked and possibleValues, which need " +
                "a placed element.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "LibraryPartNames",
                "Name of each library part, the same name CreateObjects, CreateLamps, CreateWindows and " +
                "CreateDoors take as libraryPartName.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "LibraryPartNames",
                "The name of each library part found.");

            OutTextTree(
                "ParameterNames",
                "The name of each GDL parameter (one branch per library part).");

            OutTexts(
                "Parameters",
                "The GDL parameters of each library part with their default values, as a JSON array of objects " +
                "with name, displayName, index, type, value, flags and for arrays dimension1 and dimension2 (the " +
                "Parameter Script is not run). SetLibraryPartParameters takes it back.");

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
                libraryParts.Add(new JObject { ["libraryPartName"] = name });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["libraryParts"] = libraryParts },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var outNames = new List<string>();
            var parameterNames = new DataTree<string>();
            var parameters = new List<string>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "gdlParametersOfLibraryParts");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                parameterNames.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    outNames.Add(null);
                    parameters.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                outNames.Add(item["libPart"]?["name"]?.ToString());
                if (item["parameters"] is JArray list)
                {
                    foreach (var parameter in list)
                    {
                        parameterNames.Add(parameter["name"]?.ToString(), path);
                    }
                    parameters.Add(list.ToString(Formatting.None));
                }
                else
                {
                    parameters.Add(null);
                }
                errors.Add("");
            }

            da.SetDataList(0, outNames);
            da.SetDataTree(1, parameterNames);
            da.SetDataList(2, parameters);
            da.SetDataList(3, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemGDLParameters;

        public override Guid ComponentGuid =>
            new Guid("4b1f6e05-674c-429b-a36f-bee75f143468");
    }
}
