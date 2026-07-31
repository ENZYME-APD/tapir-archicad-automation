using System;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class CreateClassificationItemsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateClassificationItems";

        public CreateClassificationItemsComponent()
            : base(
                "CreateClassificationItems",
                "Create Classification Items in the given Classification Systems based on the given parameters.",
                GroupNames.Classifications)
        {
        }

        protected override string ArrayKey => "newClassificationItems";

        protected override string InputName => "NewItems";

        protected override string InputDescription =>
            "One JSON object per classification item, e.g. " +
            "{\"classificationSystemId\":{\"guid\":\"...\"}," +
            "\"classificationItemDetails\":{\"id\":\"01\",\"name\":\"...\"}," +
            "\"parentClassificationItemId\":{\"guid\":\"...\"}}.";

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateClassificationItems;

        public override Guid ComponentGuid =>
            new Guid("9bfde227-fc58-4656-a266-40e045a5e80b");
    }
}
