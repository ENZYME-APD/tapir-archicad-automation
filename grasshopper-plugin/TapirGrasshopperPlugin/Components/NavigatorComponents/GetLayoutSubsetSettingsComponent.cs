using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetLayoutSubsetSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLayoutSubsetSettings";

        public GetLayoutSubsetSettingsComponent()
            : base(
                "GetLayoutSubsetSettings",
                "Gets the settings of existing Layout Book subsets: the same fields CreateLayoutSubset accepts " +
                "(name, ID prefix, numbering style, ...).",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemGuids",
                "Identifiers of the subset navigator items.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "Name of each subset.");

            OutTexts(
                "OwnPrefixes",
                "ID prefix of each subset.");

            OutTexts(
                "CustomNumbers",
                "Custom number of each subset.");

            OutTexts(
                "NumberingStyles",
                "Numbering style of each subset: Undefined, abc, ABC, 1, 01, 001, 0001 or noID.");

            OutIntegers(
                "StartAts",
                "Number the numbering of each subset starts at.");

            OutBooleans(
                "ContinueNumbering",
                "True when the subset continues the numbering of its parent.");

            OutBooleans(
                "UseUpperPrefixes",
                "True when the subset includes the prefix of its parent.");

            OutBooleans(
                "IncludeToIDSequences",
                "True when the subset is included in the ID sequence.");

            OutBooleans(
                "CustomNumbering",
                "True when the subset uses custom numbering.");

            OutBooleans(
                "AddOwnPrefixes",
                "True when the subset's own prefix is added to the IDs.");

            OutErrorMessages(
                "Error message of each subset (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out NavigatorItemsObject navigatorItems))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(navigatorItems),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var names = new List<object>();
            var ownPrefixes = new List<object>();
            var customNumbers = new List<object>();
            var numberingStyles = new List<object>();
            var startAts = new List<object>();
            var continueNumbering = new List<object>();
            var useUpperPrefixes = new List<object>();
            var includeToIDSequences = new List<object>();
            var customNumbering = new List<object>();
            var addOwnPrefixes = new List<object>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "subsetSettings"))
            {
                var isError = JsonOutputHelp.IsError(item);
                var settings = isError ? null : item;

                names.Add(JsonOutputHelp.Scalar(settings, "name"));
                ownPrefixes.Add(JsonOutputHelp.Scalar(settings, "ownPrefix"));
                customNumbers.Add(JsonOutputHelp.Scalar(settings, "customNumber"));
                numberingStyles.Add(JsonOutputHelp.Scalar(settings, "numberingStyle"));
                startAts.Add(JsonOutputHelp.Scalar(settings, "startAt"));
                continueNumbering.Add(JsonOutputHelp.Scalar(settings, "continueNumbering"));
                useUpperPrefixes.Add(JsonOutputHelp.Scalar(settings, "useUpperPrefix"));
                includeToIDSequences.Add(JsonOutputHelp.Scalar(settings, "includeToIDSequence"));
                customNumbering.Add(JsonOutputHelp.Scalar(settings, "customNumbering"));
                addOwnPrefixes.Add(JsonOutputHelp.Scalar(settings, "addOwnPrefix"));
                errorMessages.Add(isError ? JsonOutputHelp.ErrorMessage(item) : "");
            }

            da.SetDataList(0, names);
            da.SetDataList(1, ownPrefixes);
            da.SetDataList(2, customNumbers);
            da.SetDataList(3, numberingStyles);
            da.SetDataList(4, startAts);
            da.SetDataList(5, continueNumbering);
            da.SetDataList(6, useUpperPrefixes);
            da.SetDataList(7, includeToIDSequences);
            da.SetDataList(8, customNumbering);
            da.SetDataList(9, addOwnPrefixes);
            da.SetDataList(10, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLayoutSubsetSettings;

        public override Guid ComponentGuid =>
            new Guid("40efada7-c7a4-4eb8-b4ce-5c9126aace42");
    }
}
