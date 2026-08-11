using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateWorksheetsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateWorksheets";

        public CreateWorksheetsComponent()
            : base(
                "CreateWorksheets",
                "Create independent Worksheet databases.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the worksheets to create.");

            InTexts(
                "ReferenceIds",
                "Reference ids of the worksheets to create.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "DatabaseGuids",
                "Identifier of each created worksheet.");

            OutErrorMessages(
                "Error message of each worksheet (empty when it was created successfully).");
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

            if (!da.TryGetList(
                    1,
                    out List<string> referenceIds))
            {
                return;
            }

            if (names.Count != referenceIds.Count)
            {
                this.AddError(
                    "The size of the inputs Names and ReferenceIds must be equal.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                items.Add(
                    new JObject
                    {
                        ["name"] = names[i],
                        ["referenceId"] = referenceIds[i]
                    });
            }

            var parameters = new JObject { ["worksheetsData"] = items };

            SetCadValuesWithCreatedIds<DatabaseGuidObject>(
                CommandName,
                parameters,
                ToAddOn,
                da,
                "databases",
                "databaseId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateWorksheets;

        public override Guid ComponentGuid =>
            new Guid("a67d8a04-3d9e-4194-ba79-ad4a7f781932");
    }
}
