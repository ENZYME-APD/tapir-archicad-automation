using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateDetailsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateDetails";

        public CreateDetailsComponent()
            : base(
                "CreateDetails",
                "Create independent Detail databases.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the details to create.");

            InTexts(
                "ReferenceIds",
                "Reference ids of the details to create.");
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

            var parameters = new JObject { ["detailsData"] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDetails;

        public override Guid ComponentGuid =>
            new Guid("69711f83-06fa-44b1-87ea-84cb8c988cea");
    }
}
