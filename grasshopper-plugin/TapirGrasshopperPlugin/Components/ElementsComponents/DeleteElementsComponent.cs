using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class DeleteElementsComponent : ArchicadExecutorComponent
    {
        public DeleteElementsComponent()
            : base(
                "Delete Elements",
                "DeleteElements",
                "Delete elements",
                "Elements")
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ElementGuids",
                "ElementGuids",
                "Element ids to delete.",
                GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
        }

        protected override void Solve(
            IGH_DataAccess DA)
        {
            var elements = ElementsObj.Create(
                DA,
                0);
            if (elements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            var elementsObj = JObject.FromObject(elements);
            var response = SendArchicadAddOnCommand(
                "DeleteElements",
                elementsObj);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }

            var executionResult =
                response.Result.ToObject<ExecutionResultObj>();
            if (!executionResult.Success)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    executionResult.Error.Message);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            TapirGrasshopperPlugin.Properties.Resources.DeleteElements;

        public override Guid ComponentGuid =>
            new Guid("9880138a-731a-40f6-a820-aa47923a872f");
    }
}