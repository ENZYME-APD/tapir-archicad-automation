using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetMissingAttributesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMissingAttributes";

        public GetMissingAttributesComponent()
            : base(
                "GetMissingAttributes",
                "Lists the references of placed elements to layers, fills, surfaces, building materials, " +
                "composites, profiles, line types, pen tables and zone categories that do not exist in the " +
                "project, like the missing attributes report of the GDL palette: one item per reference, with " +
                "the element, where it sits, the attribute type, the missing index and the field or GDL " +
                "parameter that holds it. Tool defaults, Favorites, Model View Options and graphic override " +
                "rules are not checked yet. Read only. Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Types",
                GetUnusedAttributesComponent.TypesDescription);

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "The element of each missing reference.");

            OutTexts(
                "ElementTypes",
                "The type of that element.");

            OutGenerics(
                "ParentElementGuids",
                "The element it is part of (the curtain wall of a frame, the column of a segment, ...); " +
                "null when it is not part of another element.");

            OutGenerics(
                "DatabaseGuids",
                "The database (floor plan, view or layout) the element is in.");

            OutTexts(
                "Locations",
                "The story name for elements of the floor plan, else the name of the view or layout " +
                "(e.g. the worksheet).");

            OutTexts(
                "AttributeTypes",
                "The type of the missing attribute.");

            OutIntegers(
                "Indices",
                "The attribute index that no attribute of that type has.");

            OutTexts(
                "UsedBy",
                "The element's field (API name, e.g. layer) or GDL parameter that holds the index.");

            OutBooleans(
                "IsGdlParameter",
                "True when UsedBy is a GDL parameter.");

            OutInteger(
                "CheckedElementCount",
                "The number of elements checked.");

            OutTexts(
                "Warnings",
                "What could not be checked.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            var types = GetUnusedAttributesComponent.ToTypesParameter(da.GetOptionals<string>(0));
            if (types.Count > 0)
            {
                parameters["attributeTypes"] = types;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elementGuids = new List<ElementGuid>();
            var elementTypes = new List<string>();
            var parentGuids = new List<ElementGuid>();
            var databaseGuids = new List<DatabaseGuidObject>();
            var locations = new List<string>();
            var attributeTypes = new List<string>();
            var indices = new List<object>();
            var usedBy = new List<string>();
            var isGdlParameter = new List<bool>();

            foreach (var item in JsonOutputHelp.Items(response, "missingAttributes"))
            {
                elementGuids.Add(item["elementId"]?.ToObject<ElementGuid>());
                elementTypes.Add(item["elementType"]?.ToString());
                parentGuids.Add(item["parentElementId"]?.ToObject<ElementGuid>());
                databaseGuids.Add(item["databaseId"]?.ToObject<DatabaseGuidObject>());
                locations.Add(
                    item["storyName"]?.ToString() ??
                    item["databaseName"]?.ToString() ??
                    item["databaseType"]?.ToString());
                attributeTypes.Add(item["attributeType"]?.ToString());
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                var gdlParameter = item["gdlParameter"]?.ToString();
                usedBy.Add(gdlParameter ?? item["field"]?.ToString());
                isGdlParameter.Add(gdlParameter != null);
            }

            da.SetDataList(0, elementGuids);
            da.SetDataList(1, elementTypes);
            da.SetDataList(2, parentGuids);
            da.SetDataList(3, databaseGuids);
            da.SetDataList(4, locations);
            da.SetDataList(5, attributeTypes);
            da.SetDataList(6, indices);
            da.SetDataList(7, usedBy);
            da.SetDataList(8, isGdlParameter);
            da.SetData(9, JsonOutputHelp.Scalar(response, "checkedElementCount"));
            da.SetDataList(10, GetUnusedAttributesComponent.Warnings(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetUnusedAttributes;

        public override Guid ComponentGuid =>
            new Guid("3f9c6a2e-81d4-4b57-9e3a-c5d27b10f846");
    }
}
