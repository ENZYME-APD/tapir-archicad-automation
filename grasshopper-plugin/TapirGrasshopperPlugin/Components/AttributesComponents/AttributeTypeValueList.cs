using System;
using TapirGrasshopperPlugin.ResponseTypes.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class AttributeTypeValueList : ValueList
    {
        public static string Doc => "Value List for Archicad Attribute Types.";

        public AttributeTypeValueList()
            : base(
                "Attribute Type",
                "",
                Doc,
                GroupNames.Attributes)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: AttributeType.Surface);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AttributesByTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("0f6abd5b-6efe-4699-8e0f-ac67eea42890");
    }
}