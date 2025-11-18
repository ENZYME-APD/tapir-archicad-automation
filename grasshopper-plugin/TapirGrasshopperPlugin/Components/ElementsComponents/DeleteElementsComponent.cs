using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class DeleteElementsComponent : ArchicadExecutorComponent
    {
        public static string Doc => "Delete elements";
        public override string CommandName => "DeleteElements";

        public DeleteElementsComponent()
            : base(
                "Delete Elements",
                "DeleteElements",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            AddGenerics(
                "ElementGuids",
                "Elements ids to delete.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var elements = ElementsObj.Create(
                da,
                0);
            if (elements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            var elementsObj = JObject.FromObject(elements);

            if (!GetConvertedResponse(
                    CommandName,
                    elementsObj,
                    out ExecutionResultObj executionResult)) { return; }

            if (!executionResult.Success)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    executionResult.Error.Message);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteElements;

        public override Guid ComponentGuid =>
            new Guid("9880138a-731a-40f6-a820-aa47923a872f");
    }
}