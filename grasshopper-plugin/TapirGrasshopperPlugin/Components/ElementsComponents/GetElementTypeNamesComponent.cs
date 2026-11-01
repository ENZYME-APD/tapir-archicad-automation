using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementTypeNamesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementTypeNames";

        public GetElementTypeNamesComponent()
            : base(
                "GetElementTypeNames",
                "Gets the localized names of the given element types, as the running Archicad shows them " +
                "(for example 'Wand' for Wall in German Archicad). The output is aligned with the input " +
                "list of element types. Available from Archicad 27.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ElementTypes",
                "The element types to get the localized names of.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "The localized name of each element type (null for an invalid type).");

            OutErrorMessages(
                "Error message of each element type (empty when successful).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> elementTypes))
            {
                return;
            }

            var parameters = new JObject
            {
                ["elementTypes"] = JArray.FromObject(elementTypes)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var names = new List<object>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "names"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    names.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                names.Add(
                    JsonOutputHelp.Scalar(
                        item,
                        "name"));
                errors.Add("");
            }

            da.SetDataList(
                0,
                names);
            da.SetDataList(
                1,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementTypeNames;

        public override Guid ComponentGuid =>
            new Guid("0e3c51e8-6258-424a-b6fe-336dbae86340");
    }
}
