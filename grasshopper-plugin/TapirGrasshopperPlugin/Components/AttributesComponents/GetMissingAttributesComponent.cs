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
                "Lists the references of placed elements, tool defaults, Favorites, graphic override rules, " +
                "Model View Options and attributes to layers, fills, surfaces, building materials, composites, profiles, " +
                "line types, pen tables and zone categories that do not exist in the project, like the missing " +
                "attributes report of the GDL palette: one item per reference, with the source, the element " +
                "and where it sits (or the tool, Favorite, rule, Model View Options combination or attribute), the " +
                "attribute type, the missing index and the field or GDL parameter that holds it. Read only. " +
                "Available from Archicad 27.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Types",
                GetUnusedAttributesComponent.TypesDescription);

            InTexts(
                "Sources",
                "What to check: element, toolDefault, favorite, overrideRule, modelViewOption, attribute. All of them when empty.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "The element of each missing reference; null for a tool default or Favorite.");

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

            OutTexts(
                "Sources",
                "What holds each missing reference: element, toolDefault, favorite, overrideRule, modelViewOption or attribute.");

            OutTexts(
                "FavoriteNames",
                "The Favorite that holds it; null for an element or a tool default.");

            OutTexts(
                "SubElementTypes",
                "The Favorite's sub-element that holds it (e.g. a stair's structure); null otherwise.");

            OutInteger(
                "CheckedToolDefaultCount",
                "The number of tool defaults checked.");

            OutInteger(
                "CheckedFavoriteCount",
                "The number of Favorites checked.");

            OutTexts(
                "OverrideRuleNames",
                "The graphic override rule that holds it; null for anything else.");

            OutInteger(
                "CheckedOverrideRuleCount",
                "The number of graphic override rules checked.");

            OutTexts(
                "ModelViewOptionNames",
                "The Model View Options combination that holds it; null for anything else.");

            OutTexts(
                "GdlOptionsNames",
                "The library part option set of that combination whose GDL parameter holds it; null otherwise.");

            OutInteger(
                "CheckedModelViewOptionCount",
                "The number of Model View Options combinations checked.");

            OutTexts(
                "OwnerAttributeTypes",
                "The type of the attribute that holds it (composite, buildingMaterial, surface, mepSystem, " +
                "profile); null for anything else.");

            OutTexts(
                "OwnerAttributeNames",
                "The name of that attribute; null for anything else.");

            OutInteger(
                "CheckedAttributeCount",
                "The number of composites, building materials, surfaces, MEP systems and profiles checked.");
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

            var sources = new JArray();
            foreach (var source in da.GetOptionals<string>(1))
            {
                if (!string.IsNullOrWhiteSpace(source))
                {
                    sources.Add(source.Trim());
                }
            }

            if (sources.Count > 0)
            {
                parameters["sources"] = sources;
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
            var sourceList = new List<string>();
            var favoriteNames = new List<string>();
            var subElementTypes = new List<string>();
            var overrideRuleNames = new List<string>();
            var modelViewOptionNames = new List<string>();
            var gdlOptionsNames = new List<string>();
            var ownerAttributeTypes = new List<string>();
            var ownerAttributeNames = new List<string>();

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
                sourceList.Add(item["source"]?.ToString());
                favoriteNames.Add(item["favoriteName"]?.ToString());
                subElementTypes.Add(item["subElementType"]?.ToString());
                overrideRuleNames.Add(item["overrideRuleName"]?.ToString());
                modelViewOptionNames.Add(item["modelViewOptionName"]?.ToString());
                gdlOptionsNames.Add(item["gdlOptionsName"]?.ToString());
                ownerAttributeTypes.Add(item["ownerAttributeType"]?.ToString());
                ownerAttributeNames.Add(item["ownerAttributeName"]?.ToString());
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
            da.SetDataList(11, sourceList);
            da.SetDataList(12, favoriteNames);
            da.SetDataList(13, subElementTypes);
            da.SetData(14, JsonOutputHelp.Scalar(response, "checkedToolDefaultCount"));
            da.SetData(15, JsonOutputHelp.Scalar(response, "checkedFavoriteCount"));
            da.SetDataList(16, overrideRuleNames);
            da.SetData(17, JsonOutputHelp.Scalar(response, "checkedOverrideRuleCount"));
            da.SetDataList(18, modelViewOptionNames);
            da.SetDataList(19, gdlOptionsNames);
            da.SetData(20, JsonOutputHelp.Scalar(response, "checkedModelViewOptionCount"));
            da.SetDataList(21, ownerAttributeTypes);
            da.SetDataList(22, ownerAttributeNames);
            da.SetData(23, JsonOutputHelp.Scalar(response, "checkedAttributeCount"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetUnusedAttributes;

        public override Guid ComponentGuid =>
            new Guid("3f9c6a2e-81d4-4b57-9e3a-c5d27b10f846");
    }
}
