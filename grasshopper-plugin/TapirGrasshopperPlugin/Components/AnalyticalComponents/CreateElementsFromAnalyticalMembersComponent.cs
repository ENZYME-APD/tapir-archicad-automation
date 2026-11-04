using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Analytical;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class CreateElementsFromAnalyticalMembersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateElementsFromAnalyticalMembers";

        public CreateElementsFromAnalyticalMembersComponent()
            : base(
                "CreateElementsFromAnalyticalMembers",
                "Creates elements (beams, columns, walls, slabs, roofs) from the given curve, surface or " +
                "segmented surface members, in the form GetAnalyticalMemberDetails returns them, optionally with " +
                "the settings of a favorite, in one undo step. Returns the new elements and, from Archicad 26, " +
                "the mapping of their edges and vertices to member parts.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Members",
                "Each member to create an element from as JSON, as the Members output of " +
                "GetAnalyticalMemberDetails gives it: {\"curveMember\": ...} (a beam or a column), " +
                "{\"surfaceMember\": ...} (a wall, a slab or a roof) or {\"segmentedSurfaceMembers\": [...]} " +
                "(a curved wall). The elemGuid of a member is ignored; a curve member needs analyticalMemberType " +
                "and curve, a surface member analyticalMemberType and contour.");

            InTexts(
                "FavoriteNames",
                "Name of the favorite whose settings each new element gets " +
                "(input only 1 to use the same favorite for all). Optional.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of the element created from each member.");

            OutTexts(
                "IdMappings",
                "The mapping of the edges and vertices of each member to the parts of the new element as JSON " +
                "(Archicad 26 and newer; null when there is none).");

            OutErrorMessages(
                "Error message of each member (empty when its element was created).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<string> texts) ||
                !AnalyticalJsonHelp.TryParseObjects(this, texts, "Members", out List<JObject> items))
            {
                return;
            }

            da.TryGetList(1, out List<string> favoriteNames);
            favoriteNames = favoriteNames ?? new List<string>();
            if (!AnalyticalHelp.CheckOptionalCount(this, favoriteNames.Count, items.Count, "FavoriteNames", "Members"))
            {
                return;
            }

            var members = new JArray();
            for (var i = 0; i < items.Count; i++)
            {
                JObject member = null;
                foreach (var pair in AnalyticalJsonHelp.MemberKinds)
                {
                    if (items[i][pair.Key] != null)
                    {
                        // The connections and the id mapping that
                        // GetAnalyticalMemberDetails also returns are left out.
                        member = AnalyticalJsonHelp.Pick(items[i], pair.Key);
                        break;
                    }
                }
                if (member == null)
                {
                    this.AddError(
                        $"Item {i} of the input Members has no curveMember, surfaceMember or segmentedSurfaceMembers.");
                    return;
                }
                if (favoriteNames.Count > 0)
                {
                    var favoriteName = AnalyticalHelp.ItemFor(favoriteNames, i);
                    if (!string.IsNullOrEmpty(favoriteName))
                    {
                        member["favoriteName"] = favoriteName;
                    }
                }
                members.Add(member);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["members"] = members },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            SetCreatedIdsAndErrorMessages<ElementGuid>(
                da,
                response,
                "elements",
                "elementId",
                0,
                2);

            var idMappings = new List<string>();
            foreach (var item in JsonOutputHelp.Items(response, "elements"))
            {
                idMappings.Add(
                    JsonOutputHelp.IsError(item)
                        ? null
                        : AnalyticalJsonHelp.TextOf(item["idMapping"]));
            }
            da.SetDataList(1, idMappings);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAnalyticalLoadGroups;

        public override Guid ComponentGuid =>
            new Guid("c3afecb4-af79-427e-b0c9-37c1cf967bf9");
    }
}
