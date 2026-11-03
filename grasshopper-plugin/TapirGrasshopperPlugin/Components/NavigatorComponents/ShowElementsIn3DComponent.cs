using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class ShowElementsIn3DComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ShowElementsIn3D";

        public ShowElementsIn3DComponent()
            : base(
                "ShowElementsIn3D",
                "Open the 3D window on the given elements, as Show Selection in 3D does: they become " +
                "the selection and only they are shown. Without elements it shows every element, " +
                "as Show All in 3D does.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Element ids to show in 3D. Empty: every element is shown.");

            SetOptionality(
                new[]
                {
                    0
                });
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
            ElementsObject input;

            // An unconnected or empty input sends no elements: Show All in 3D.
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> wrappers) ||
                wrappers.Count == 0)
            {
                input = new ElementsObject
                {
                    Elements = new List<ElementGuidWrapper>()
                };
            }
            else if (!wrappers.TryBuildObject(out input))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
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

            da.SetData(0, response.Message());
        }

        // Shares the icon of ZoomToElements until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ZoomToElements;

        public override Guid ComponentGuid =>
            new Guid("a7e14c58-2b9f-4d63-8e07-5f3c1b9d2a76");
    }
}
