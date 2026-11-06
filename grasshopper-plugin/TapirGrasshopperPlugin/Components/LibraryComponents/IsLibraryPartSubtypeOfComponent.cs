using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.NavigatorComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class IsLibraryPartSubtypeOfComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "IsLibraryPartSubtypeOf";

        public IsLibraryPartSubtypeOfComponent()
            : base(
                "IsLibraryPartSubtypeOf",
                "Checks for each of the given library parts whether the given ancestor is anywhere up its subtype " +
                "hierarchy (e.g. whether a part is a kind of furniture), independent of its name. A library part is " +
                "not a subtype of itself. The outputs list the parts given by index first, then by name, then by " +
                "unique ID.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InIntegers(
                "Indices",
                LibraryPartIdentifierHelp.IndicesDescription);

            InTexts(
                "Names",
                LibraryPartIdentifierHelp.NamesDescription);

            InTexts(
                "UnIds",
                LibraryPartIdentifierHelp.UnIdsDescription);

            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "AncestorIndex",
                "The index of the subtype to check against. Give exactly one of AncestorIndex, AncestorName and " +
                "AncestorUnId.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "AncestorName",
                "The name of the loaded library part (subtype) to check against, as GetAvailableLibraryParts " +
                "returns it in documentName.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "AncestorUnId",
                "The full unique ID of the subtype to check against. It is used as it is, without checking that " +
                "such a library part is loaded.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "MatchAncestorByMainId",
                "True to compare only the main GUID of the ancestor's unique ID, so any revision of the ancestor " +
                "matches. False by default: the full unique ID must match. Optional.");

            SetOptionality(new[] { 0, 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutText(
                "AncestorUnId",
                "The full unique ID of the ancestor the library parts were checked against.");

            OutTexts(
                "UnIds",
                "The full unique ID of each checked library part.");

            OutBooleans(
                "IsSubtypeOf",
                "True if the ancestor is anywhere up the subtype hierarchy of the library part; false for the " +
                "ancestor itself.");

            OutErrorMessages(
                "Error message of each library part (empty when it was checked).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var libraryParts = LibraryPartIdentifierHelp.Identifiers(da, 0);
            if (libraryParts.Count == 0)
            {
                return;
            }

            var ancestor = new JObject();
            NavigatorComponentHelp.CopyOptional<int>(da, 3, ancestor, "index");
            NavigatorComponentHelp.CopyOptional<string>(da, 4, ancestor, "name");
            NavigatorComponentHelp.CopyOptional<string>(da, 5, ancestor, "unId");
            if (ancestor.Count != 1)
            {
                this.AddError("Give exactly one of AncestorIndex, AncestorName and AncestorUnId.");
                return;
            }

            var parameters = new JObject
            {
                ["libraryParts"] = libraryParts,
                ["ancestor"] = ancestor
            };

            NavigatorComponentHelp.CopyOptional<bool>(da, 6, parameters, "matchAncestorByMainId");

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var unIds = new List<object>();
            var isSubtypeOf = new List<object>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "results"))
            {
                var result = JsonOutputHelp.IsError(item) ? null : item;
                unIds.Add(JsonOutputHelp.Scalar(result, "unId"));
                isSubtypeOf.Add(JsonOutputHelp.Scalar(result, "isSubtypeOf"));
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetData(0, response["ancestorUnId"]?.ToString());
            da.SetDataList(1, unIds);
            da.SetDataList(2, isSubtypeOf);
            da.SetDataList(3, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CompareLibraryPartUnIds;

        public override Guid ComponentGuid =>
            new Guid("52bf767f-bdba-4cd4-8fba-823c829a8b97");
    }
}
