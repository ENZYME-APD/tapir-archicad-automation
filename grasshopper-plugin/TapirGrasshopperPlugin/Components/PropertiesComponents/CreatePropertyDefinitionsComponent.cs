using System;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class CreatePropertyDefinitionsComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreatePropertyDefinitions";

        public CreatePropertyDefinitionsComponent()
            : base(
                "CreatePropertyDefinitions",
                "Create Custom Property Definitions based on the given parameters.",
                GroupNames.Properties)
        {
        }

        protected override string ArrayKey => "propertyDefinitions";

        protected override string ItemWrapKey => "propertyDefinition";

        protected override string InputName => "PropertyDefinitions";

        protected override string InputDescription =>
            "One JSON object per property definition, e.g. " +
            "{\"name\":\"MyProperty\",\"description\":\"...\",\"type\":\"string\"," +
            "\"isEditable\":true,\"defaultValue\":{...},\"availability\":[...],\"group\":{\"name\":\"My Group\"}}.";

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreatePropertyDefinitions;

        public override Guid ComponentGuid =>
            new Guid("9d6d8b64-9be1-4606-bb75-f64d8d2b49f3");
    }
}
