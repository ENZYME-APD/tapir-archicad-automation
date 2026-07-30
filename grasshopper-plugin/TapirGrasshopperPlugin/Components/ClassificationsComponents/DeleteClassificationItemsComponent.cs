using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class DeleteClassificationItemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteClassificationItems";

        public DeleteClassificationItemsComponent()
            : base(
                "DeleteClassificationItems",
                "Delete the given Classification Items.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ClassificationItemGuids",
                "Identifiers of the classification items to delete.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            var input = new DeleteClassificationItemsParameters
            {
                ClassificationItemIds = new List<ClassificationItemGuidWrapper>()
            };

            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<ClassificationGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid classification item identifier in the ClassificationItemGuids input.");
                    return;
                }
                input.ClassificationItemIds.Add(
                    new ClassificationItemGuidWrapper { ClassificationItemId = id });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteClassificationItems;

        public override Guid ComponentGuid =>
            new Guid("e0ad2b8f-3bde-4684-a303-8daf7dd6bac3");
    }
}
