using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ZoomToElementsComponent : ArchicadExecutorComponent
    {
        // ZoomToElements zooms to the elements in the 3D window too, which FitInWindow's
        // element list does not; it needs elements, so with none given FitInWindow fits
        // the whole drawing as before.
        public override string CommandName => "ZoomToElements";

        public ZoomToElementsComponent()
            : base(
                "ZoomToElements",
                "Zooms the current window, 2D or 3D, to the given elements: selects them, zooms to the selection " +
                "and restores the previous selection unless KeepSelection is false. Fails when none of the elements " +
                "could be selected, e.g. all of them are on another story or on a hidden layer. If no element is " +
                "given, it works as 'fit in window', zooms to all elements in the project.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Element ids to zoom to.");

            // Appended: saved definitions find the inputs by position.
            inManager.AddBooleanParameter(
                "KeepSelection",
                "KeepSelection",
                ("Restore the selection the window had before the command (default). With false the given " +
                 "elements stay selected. Optional.").WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject input))
            {
                return;
            }

            ExecutionResult response;

            if (input.Elements != null && input.Elements.Count > 0)
            {
                var parameters = JObject.FromObject(input);

                var keepSelection = true;
                if (da.GetData(
                        1,
                        ref keepSelection))
                {
                    parameters["keepSelection"] = keepSelection;
                }

                if (!TryGetConvertedCadValues(
                        CommandName,
                        parameters,
                        ToAddOn,
                        ExecutionResult.Deserialize,
                        out response))
                {
                    return;
                }
            }
            else if (!TryGetConvertedCadValues(
                         "FitInWindow",
                         new JObject(),
                         ToAddOn,
                         ExecutionResult.Deserialize,
                         out response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ZoomToElements;

        public override Guid ComponentGuid =>
            new Guid("30bf1eeb-6542-4d37-81ea-20d5cff55d72");
    }
}