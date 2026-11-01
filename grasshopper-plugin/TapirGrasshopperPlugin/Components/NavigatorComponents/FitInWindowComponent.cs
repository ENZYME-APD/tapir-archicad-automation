using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class FitInWindowComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "FitInWindow";

        public FitInWindowComponent()
            : base(
                "FitInWindow",
                "Zooms to the given elements or fits everything in the window.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements to zoom to. Optional, without elements everything is fitted in the window.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryCreateFromList(
                    0,
                    out ElementsObject elements) &&
                elements.Elements != null &&
                elements.Elements.Count > 0)
            {
                parameters = JObject.FromObject(elements);
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(
                0,
                response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FitInWindow;

        public override Guid ComponentGuid =>
            new Guid("02afcb48-e016-45b8-a47f-c9e3d832dd7a");
    }
}
