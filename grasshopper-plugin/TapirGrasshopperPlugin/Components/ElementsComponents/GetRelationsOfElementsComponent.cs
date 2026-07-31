using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetRelationsOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetRelationsOfElements";

        public GetRelationsOfElementsComponent()
            : base(
                "RelationsOfElements",
                "Gets the type-specific relations of the given elements: endpoint and reference line connections of walls, beams and beam segments, boundary elements of zones, the zones on the two sides of openings, and the zones connected to roofs and shells. Available from Archicad 26.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");

            InText(
                "OtherElementType",
                "Optional filter: only relations to elements of this type are returned.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "RelatedElementGuids",
                "Identifiers of the related elements (one branch per input element).");

            OutTexts(
                "RelationDetails",
                "Full relation details of each input element as JSON (connection sides, zone boundary sections, etc.).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried element (empty when successful).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new ElementTypeValueList(ElementTypeValueListType.AllElements)
                .AddAsSource(
                    this,
                    1);
        }

        private static void CollectGuids(
            JToken token,
            List<string> guids)
        {
            if (token is JObject obj)
            {
                var guid = obj["guid"]?.ToString();
                if (guid != null)
                {
                    guids.Add(guid);
                    return;
                }

                foreach (var property in obj.Properties())
                {
                    CollectGuids(property.Value, guids);
                }
            }
            else if (token is JArray array)
            {
                foreach (var item in array)
                {
                    CollectGuids(item, guids);
                }
            }
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject input))
            {
                return;
            }

            var parameters = JObject.FromObject(input);
            string otherElementType = null;
            if (da.GetData(1, ref otherElementType) && !string.IsNullOrEmpty(otherElementType))
            {
                parameters["otherElementType"] = otherElementType;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var relatedElementGuids = new DataTree<object>();
            var relationDetails = new List<string>();
            var errors = new List<string>();

            var items = response["relations"] as JArray ?? new JArray();
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                relatedElementGuids.EnsurePath(path);

                if (item?["error"] != null)
                {
                    errors.Add(item["error"]?["message"]?.ToString() ?? "");
                    relationDetails.Add(null);
                    continue;
                }

                errors.Add("");
                relationDetails.Add(item?.ToString(Formatting.None));

                var guids = new List<string>();
                CollectGuids(item, guids);
                foreach (var guid in guids)
                {
                    relatedElementGuids.Add(
                        new ElementGuidWrapper
                        {
                            ElementId = new ElementGuid { Guid = guid }
                        },
                        path);
                }
            }

            da.SetDataTree(0, relatedElementGuids);
            da.SetDataList(1, relationDetails);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetRelationsOfElements;

        public override Guid ComponentGuid =>
            new Guid("de3f942e-594a-4358-a4f5-004b0a904445");
    }
}
