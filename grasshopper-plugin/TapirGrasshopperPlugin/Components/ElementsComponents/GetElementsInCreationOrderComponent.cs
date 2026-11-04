using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementsInCreationOrderComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementsInCreationOrder";

        public GetElementsInCreationOrderComponent()
            : base(
                "GetElementsInCreationOrder",
                "Returns the identifier of every element of the current database, or of the given databases, " +
                "in the order the elements were created.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "Databases",
                "Optional. The databases to list the elements of, each in its own creation order. If " +
                "omitted, the current database is used.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "The elements in creation order (the databases one after the other).");

            OutErrorMessages(
                "Error message of each given database (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            var databases = DatabasesObject.Create(
                da,
                0);
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

        // Shares the icon of GetElementsInDrawOrder until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementsInDrawOrder;

        public override Guid ComponentGuid =>
            new Guid("de5a4763-d55f-4580-8e66-ef1e205f065f");
    }
}
