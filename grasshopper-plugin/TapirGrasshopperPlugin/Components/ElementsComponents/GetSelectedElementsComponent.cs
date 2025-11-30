using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetSelectedElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetSelectedElements";

        public GetSelectedElementsComponent()
            : base(
                "SelectedElements",
                "Get currently selected elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Currently selected element Guids.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedValues(
                    CommandName,
                    null,
                    SendToAddOn,
                    JHelp.Deserialize<ElementsObj>,
                    out ElementsObj response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Elements);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SelectedElems;

        public override Guid ComponentGuid =>
            new Guid("1949E4B5-4E37-4F35-8C5C-BEA7575AC1C6");
    }
}