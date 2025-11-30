using Grasshopper.Kernel.Special;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class ClassificationSystemValueList : ArchicadAccessorValueList
    {
        public override string CommandName => "GetAllClassificationSystems";

        public ClassificationSystemValueList()
            : base(
                "ClassificationSystems",
                "Value list for Classification Systems.",
                GroupNames.Classifications)
        {
        }

        public override void RefreshItems()
        {
            var response = SendArchicadCommand(
                CommandName,
                null);

            if (!response.Succeeded)
            {
                this.AddError(response.GetErrorMessage());
                return;
            }

            var previouslySelected = SelectedItems[0].Expression;

            ListItems.Clear();

            var sytems = response.Result.ToObject<AllClassificationSystems>();

            foreach (var system in sytems.ClassificationSystems)
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