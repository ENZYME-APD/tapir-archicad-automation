using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementsInDrawOrderComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementsInDrawOrder";

        public GetElementsInDrawOrderComponent()
            : base(
                "GetElementsInDrawOrder",
                "Returns the identifier of every element of the current database, or of the given databases, " +
                "in Archicad's drawing order (the order set by Bring to Front and Send to Back). Use the " +
                "optional ElementType and Filters inputs for filtering. Objects that belong to another " +
                "element (zone stamps, the parts of stairs and railings) are left out unless the " +
                "IncludeSubElemObjects filter is given.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ElementType",
                "Optional. Only elements of this type are returned, still in drawing order. If omitted, " +
                "every element is returned.");

            InTexts(
                "Filters",
                "Optional. The filters an element must pass to be returned, the same as for " +
                "GetElementsByType.");

            InGenerics(
                "Databases",
                "Optional. The databases to list the elements of, each in its own drawing order. If " +
                "omitted, the current database is used.");

            SetOptionality(
                new[]
                {
                    0,
                    1,
                    2
                });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "The elements in drawing order (the databases one after the other).");

            OutErrorMessages(
                "Error message of each given database (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            string elementType = null;
            if (da.GetData(
                    0,
                    ref elementType) &&
                !string.IsNullOrEmpty(elementType))
            {
                parameters["elementType"] = elementType;
            }

            var filters = new AcceptsElementFilters().AcceptElementFilters(
                da.GetOptionals<string>(1));
            if (filters != null)
            {
                parameters["filters"] = JArray.FromObject(filters);
            }

            var databases = DatabasesObject.Create(
                da,
                2);
            if (databases?.Databases != null && databases.Databases.Count > 0)
            {
                parameters["databases"] = JArray.FromObject(databases.Databases);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elements = new List<object>();
            if (response["elements"] is JArray items)
            {
                foreach (var item in items)
                {
                    elements.Add(ElementsStructuredGetterComponent.ElementIdOf(item));
                }
            }

            da.SetDataList(
                0,
                elements);
            da.SetDataList(
                1,
                ExecutionResultsResponse.ErrorMessages(
                    response,
                    "executionResultForDatabases"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementsInDrawOrder;

        public override Guid ComponentGuid =>
            new Guid("4cf626ec-4205-450d-a708-c648f2ccd892");
    }
}
