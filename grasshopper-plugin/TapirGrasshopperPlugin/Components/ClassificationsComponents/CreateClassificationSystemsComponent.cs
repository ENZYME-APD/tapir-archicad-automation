using System;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class CreateClassificationSystemsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateClassificationSystems";

        public CreateClassificationSystemsComponent()
            : base(
                "CreateClassificationSystems",
                "Create Classification Systems including Classification Items based on the given parameters.",
                GroupNames.Classifications,
                "classificationSystemsWithItems",
                "SystemsWithItems",
                "One JSON object per classification system, e.g. " +
                "{\"classificationSystem\":{\"name\":\"MySystem\",\"version\":\"1.0\"}," +
                "\"classificationItems\":[{\"classificationItem\":{\"id\":\"01\",\"name\":\"...\",\"children\":[...]}}]}.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateClassificationSystems;

        public override Guid ComponentGuid =>
            new Guid("061c6a1b-9f40-41db-befc-639018a6344f");
    }
}
