using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetModelViewOptionDetailsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetModelViewOptionDetails";

        public GetModelViewOptionDetailsComponent()
            : base(
                "GetModelViewOptionDetails",
                "Gets the settings stored in model view option combinations, all of them or the ones given by id or " +
                "by name: the model view options and the options library parts add to the combination, with their " +
                "GDL parameter values. Read-only. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ModelViewOptionGuids",
                "The identifiers (GUIDs) of the model view option combinations to read. Give these or Names; " +
                "without both every combination is read.");

            InTexts(
                "Names",
                "The names of the model view option combinations to read, as shown in " +
                "Document > Model View > Model View Options. Give these or ModelViewOptionGuids.");

            SetOptionality(
                new[]
                {
                    0,
                    1
                });
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "Indices",
                "The index of each model view option combination, the one SetViewOptions takes.");

            OutTexts(
                "ModelViewOptionGuids",
                "The identifier of each model view option combination.");

            OutTexts(
                "Names",
                "The name of each model view option combination.");

            OutTexts(
                "ModelViewOptions_JSON",
                "The model view options of each combination as a JSON object, in the fields of GetViewOptions. " +
                "Values are the numeric API constants; attribute indices are plain integers.");

            OutTexts(
                "GdlOptions_JSON",
                "The options library parts add to each combination, as a JSON array of objects with name, guid, " +
                "supersetGuid and parameters.");

            OutErrorMessages(
                "Error message of each requested combination (empty when it was found).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGetList(
                0,
                out List<string> guids);

            da.TryGetList(
                1,
                out List<string> names);

            guids = guids.Where(g => !string.IsNullOrWhiteSpace(g)).ToList();
            names = names.Where(n => !string.IsNullOrEmpty(n)).ToList();

            if (guids.Count > 0 && names.Count > 0)
            {
                this.AddError(
                    "Give ModelViewOptionGuids or Names, not both.");
                return;
            }

            var parameters = new JObject();

            if (guids.Count > 0 || names.Count > 0)
            {
                var modelViewOptions = new JArray();

                foreach (var guid in guids)
                {
                    modelViewOptions.Add(
                        new JObject
                        {
                            ["modelViewOptionId"] = NavigatorComponentHelp.IdObject(guid.Trim())
                        });
                }

                foreach (var name in names)
                {
                    modelViewOptions.Add(
                        new JObject
                        {
                            ["name"] = name
                        });
                }

                parameters["modelViewOptions"] = modelViewOptions;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var indices = new List<int?>();
            var ids = new List<string>();
            var foundNames = new List<string>();
            var modelViewOptionsJson = new List<string>();
            var gdlOptionsJson = new List<string>();
            var errors = new List<string>();

            foreach (var details in response["modelViewOptionDetails"] ?? new JArray())
            {
                var error = details["error"]?["message"]?.ToString();
                indices.Add(error == null ? details.Value<int?>("index") : null);
                ids.Add(error == null ? JsonOutputHelp.GuidOf(details["modelViewOptionId"]) : null);
                foundNames.Add(error == null ? details["name"]?.ToString() : null);
                modelViewOptionsJson.Add(error == null ? NavigatorComponentHelp.JsonText(details["modelViewOptions"]) : null);
                gdlOptionsJson.Add(error == null ? NavigatorComponentHelp.JsonText(details["gdlOptions"]) : null);
                errors.Add(error ?? string.Empty);
            }

            da.SetDataList(0, indices);
            da.SetDataList(1, ids);
            da.SetDataList(2, foundNames);
            da.SetDataList(3, modelViewOptionsJson);
            da.SetDataList(4, gdlOptionsJson);
            da.SetDataList(5, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModelViewOptions;

        public override Guid ComponentGuid =>
            new Guid("be101edd-5789-4188-9be5-f47a48407a47");
    }
}
