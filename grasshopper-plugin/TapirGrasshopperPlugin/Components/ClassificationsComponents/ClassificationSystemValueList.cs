using Grasshopper.Kernel.Special;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class ClassificationSystemValueList : ArchicadAccessorValueList
    {
        public static string Doc => "Value List for Classification Systems.";
        public override string CommandName => "GetAllClassificationSystems";

        public ClassificationSystemValueList()
            : base(
                "Classification Systems",
                "",
                Doc,
                GroupNames.Classifications)
        {
        }

        public override void RefreshItems()
        {
            if (!GetAndConvertResponse(
                    CommandName,
                    null,
                    out AllClassificationSystems classificationSystems))
            {
                return;
            }

            var previouslySelected = SelectedItems[0].Expression;

            ListItems.Clear();

            foreach (var system in classificationSystems.ClassificationSystems)
            {
                var item = new GH_ValueListItem(
                    system.ToString(),
                    '"' + system.ClassificationSystemId.Guid + '"');
                if (item.Expression == previouslySelected)
                {
                    item.Selected = true;
                }

                ListItems.Add(item);
            }

            if (SelectedItems.Count == 0)
            {
                ListItems[0].Selected = true;
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ClassificationSystemsValueList;

        public override Guid ComponentGuid =>
            new Guid("a4206a77-3e1e-42e1-b220-dbd7aafdf8f5");
    }
}