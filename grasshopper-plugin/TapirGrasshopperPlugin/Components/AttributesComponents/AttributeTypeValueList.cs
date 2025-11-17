using System;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public enum AttributeType
    {
        Layer,
        Line,
        Fill,
        Composite,
        Surface,
        LayerCombination,
        ZoneCategory,
        Profile,
        PenTable,
        MEPSystem,
        OperationProfile,
        BuildingMaterial
    }

    public class AttributeTypeValueList : ValueList
    {
        public AttributeTypeValueList () :
            base ("Attribute Type", "",
                "Value List for Archicad Attribute Types.", "Attributes")
        {
        }

        public override void RefreshItems ()
        {
            ListItems.Clear ();
            AddEnumItems (defaultSelected: AttributeType.Surface);
            ExpireSolution (true);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.AttributesByTypeValueList;

        public override Guid ComponentGuid => new Guid ("0f6abd5b-6efe-4699-8e0f-ac67eea42890");
    }
}
