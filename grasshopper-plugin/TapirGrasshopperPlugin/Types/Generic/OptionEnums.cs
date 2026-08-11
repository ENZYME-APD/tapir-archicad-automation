namespace TapirGrasshopperPlugin.Types.Generic
{
    // The option sets of the string valued command parameters. They exist so the
    // matching value lists can be generated from them, keeping the offered options
    // in sync with what the commands accept.

    public enum StructureType
    {
        Basic,
        Composite,
        Profile
    }

    public enum ReferenceLineLocation
    {
        Outside,
        Center,
        Inside,
        CoreOutside,
        CoreCenter,
        CoreInside
    }

    public enum ReferencePlaneLocation
    {
        Top,
        CoreTop,
        CoreBottom,
        Bottom
    }

    public enum AnchorPoint
    {
        TopLeft,
        TopCenter,
        TopRight,
        MiddleLeft,
        Center,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    }

    public enum TextJustification
    {
        Left,
        Center,
        Right,
        Full
    }

    public enum MeshSkirtType
    {
        SurfaceOnlyWithoutSkirt,
        WithSkirt,
        SolidBodyWithSkirt
    }

    public enum MeshRidgeType
    {
        AllSharp,
        AllSmooth,
        UserDefined
    }

    public enum DimensioningPreset
    {
        WallCompositeFaces,
        WallSkinBorders,
        SlabCompositeFaces,
        SlabSkinBorders,
        BeamOrColumnRefLineEndPoints,
        BeamOrColumnBoundingBoxCorners,
        DoorWindowWallHoleCorners,
        DoorWindowModelHotspots
    }

    public enum MEPDomain
    {
        Piping,
        Ventilation
    }
}
