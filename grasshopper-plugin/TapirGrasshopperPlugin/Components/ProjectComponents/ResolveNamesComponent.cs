using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class ResolveNamesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "ResolveNames";

        private const string MatchKeysDoc =
            "Each match is JSON with kind, name, match (exact, normalized, words, contains or similar) and score, " +
            "and what the other commands take: storyIndex and level for a story; attributeId and attributeIndex for " +
            "an attribute; elementType for an element type; propertyId with propertyGroupId, propertyGroupName, " +
            "propertyType, propertyValueType and availability for a property; classificationSystemId and " +
            "classificationItemId for a classification.";

        public ResolveNamesComponent()
            : base(
                "ResolveNames",
                "Looks up words or phrases among the names of the open project (stories, attributes, element types, " +
                "properties and classifications) before they are passed to other components. For each name it says " +
                "whether items are named exactly so (found), only nearly so (closeMatches, best first) or nothing " +
                "is close (notFound: the project has no such item and it must not be guessed). " +
                "Matching is lexical and deterministic. Read only.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "The words or phrases to look up.");

            InTexts(
                "Kinds",
                "The kind of item each name names (input only 1 to use the same for all): Story, Layer, " +
                "LayerCombination, BuildingMaterial, Composite, Profile, Surface, Fill, Line, ZoneCategory, " +
                "ElementType, Property or Classification. Optional: an empty item, or no input, searches every kind.");

            inManager.AddIntegerParameter(
                "MaxCloseMatches",
                "MaxCloseMatches",
                "The most close matches returned for one name, best first (1 to 100). Optional; default: 5."
                    .WithTypeName("integerItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Statuses",
                "The result of each name: found, closeMatches or notFound.");

            OutTextTree(
                "Found",
                "The items named exactly so, one branch per name, as JSON. More than one when items of different " +
                "kinds, or several stories, have the name. " +
                MatchKeysDoc);

            OutTextTree(
                "CloseMatches",
                "When nothing is named exactly so: the nearest items, best first, one branch per name, as JSON. " +
                MatchKeysDoc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            var kinds = da.GetOptionals<string>(1);
            if (kinds.Count > 1 &&
                kinds.Count != names.Count)
            {
                this.AddError(
                    "The count of Kinds must be 0, 1 or the same as the count of Names.");
                return;
            }

            var nameItems = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var item = new JObject { ["name"] = names[i] ?? "" };
                var kind = kinds.Count == 0
                    ? null
                    : kinds.Count == 1
                        ? kinds[0]
                        : kinds[i];
                if (!string.IsNullOrEmpty(kind))
                {
                    item["kind"] = kind;
                }

                nameItems.Add(item);
            }

            var parameters = new JObject { ["names"] = nameItems };

            int maxCloseMatches = 0;
            if (da.GetData(2, ref maxCloseMatches))
            {
                parameters["maxCloseMatches"] = maxCloseMatches;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var statuses = new List<string>();
            var found = new DataTree<string>();
            var closeMatches = new DataTree<string>();

            var results = JsonOutputHelp.Items(
                response,
                "results");
            for (var i = 0; i < results.Count; i++)
            {
                var result = results[i];
                var path = new GH_Path(i);
                found.EnsurePath(path);
                closeMatches.EnsurePath(path);

                statuses.Add(result["status"]?.ToString());

                if (result["found"] is JArray foundItems)
                {
                    foreach (var match in foundItems)
                    {
                        found.Add(
                            match.ToString(Formatting.None),
                            path);
                    }
                }

                if (result["closeMatches"] is JArray closeItems)
                {
                    foreach (var match in closeItems)
                    {
                        closeMatches.Add(
                            match.ToString(Formatting.None),
                            path);
                    }
                }
            }

            da.SetDataList(0, statuses);
            da.SetDataTree(1, found);
            da.SetDataTree(2, closeMatches);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ResolveNames;

        public override Guid ComponentGuid =>
            new Guid("48b1ef4b-ed1c-4398-a3fa-bd2ff5ca1118");
    }
}
