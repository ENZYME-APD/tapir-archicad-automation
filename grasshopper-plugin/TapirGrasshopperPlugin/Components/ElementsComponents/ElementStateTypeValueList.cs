using Grasshopper.Kernel.Special;
using System;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ElementStateTypeValueList : ValueList
    {
        public ElementStateTypeValueList()
            : base(
                "ElementStateType",
                "Value list for the types of CaptureElementState.",
                GroupNames.Elements)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();

            // Not an enum: 3DModel is no C# identifier.
            foreach (var type in ElementStateTypes.All)
            {
                ListItems.Add(
                    new GH_ValueListItem(
                            type,
                            '"' + type + '"')
                        {
                            Selected = type == ElementStateTypes.ModificationStamp
                        });
            }

            ExpireSolution(true);
        }

        // No icon of its own yet: the one of the element filter value list.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemFilterValueList;

        public override Guid ComponentGuid =>
            new Guid("9d4b2f67-c3a8-4e15-a7f0-14e6d8b3c25a");
    }
}
