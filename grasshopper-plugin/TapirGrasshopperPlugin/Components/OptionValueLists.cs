using System;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components
{
    // Value lists for the string valued inputs that only accept a fixed set of
    // options, so the exact spelling never has to be typed by hand. The creation
    // and modification components attach the matching one automatically when they
    // are dropped on the canvas (see CreateElementsComponentBase.AddedToDocument).

    public class StructureTypeValueList : ValueList
    {
        public StructureTypeValueList()
            : base(
                "StructureTypes",
                "Value list for the structure type of walls, slabs and roofs.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: StructureType.Basic);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("2b5c1c04-06f6-4d0e-9a2e-6f6f3d51c5a1");
    }

    public class ReferenceLineLocationValueList : ValueList
    {
        public ReferenceLineLocationValueList()
            : base(
                "ReferenceLineLocations",
                "Value list for the reference line location of walls.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: ReferenceLineLocation.Center);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("8d1a58f0-6b1e-4b31-9a3a-2e6cbb0a4f77");
    }

    public class ReferencePlaneLocationValueList : ValueList
    {
        public ReferencePlaneLocationValueList()
            : base(
                "ReferencePlaneLocations",
                "Value list for the reference plane location of slabs.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: ReferencePlaneLocation.Top);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("bf4a1de4-3d59-4a67-9c65-c3a5cf9e0a12");
    }

    public class AnchorPointValueList : ValueList
    {
        public AnchorPointValueList()
            : base(
                "AnchorPoints",
                "Value list for the cross section anchor point of beams and columns.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: AnchorPoint.Center);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("5a3d9b7e-1f2c-4f8b-89f4-6f1d3a0b7c25");
    }

    public class TextJustificationValueList : ValueList
    {
        public TextJustificationValueList()
            : base(
                "TextJustifications",
                "Value list for the justification of text elements.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: TextJustification.Left);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("c7e0b8a1-4d63-4a0e-8b5c-9f2a6d4e1b30");
    }

    public class MeshSkirtTypeValueList : ValueList
    {
        public MeshSkirtTypeValueList()
            : base(
                "MeshSkirtTypes",
                "Value list for the skirt type of mesh elements.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: MeshSkirtType.WithSkirt);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("9f6c2a54-8e17-4c0d-b3a2-71d5c8e4f096");
    }

    public class MeshRidgeTypeValueList : ValueList
    {
        public MeshRidgeTypeValueList()
            : base(
                "MeshRidgeTypes",
                "Value list for the ridge type of mesh elements.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: MeshRidgeType.AllSharp);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("1c8f47b9-52a6-4e3f-9d10-4b6e2f8a53c4");
    }

    public class DimensioningPresetValueList : ValueList
    {
        public DimensioningPresetValueList()
            : base(
                "DimensioningPresets",
                "Value list for the preset of associative dimensions on section databases.",
                GroupNames.ElementCreation)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: DimensioningPreset.WallCompositeFaces);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("6e2b0d3f-7c94-4a15-8f6b-0a3d9c5e72b8");
    }

    public class MEPDomainValueList : ValueList
    {
        public MEPDomainValueList()
            : base(
                "MEPDomains",
                "Value list for the MEP domain of the segment preference tables.",
                GroupNames.MEP)
        {
        }

        public override void RefreshItems()
        {
            ListItems.Clear();
            AddEnumItems(defaultSelected: MEPDomain.Piping);
            ExpireSolution(true);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemTypeValueList;

        public override Guid ComponentGuid =>
            new Guid("3a9d6f10-b845-4c72-9e03-5d1f7b6a4e29");
    }
}
