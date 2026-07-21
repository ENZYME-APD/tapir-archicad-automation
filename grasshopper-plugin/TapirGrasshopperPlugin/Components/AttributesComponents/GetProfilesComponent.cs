using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetProfilesComponent : AttributeStructuredGetterComponent
    {
        public override string CommandName => "GetProfiles";

        public GetProfilesComponent()
            : base(
                "GetProfiles",
                "Get the details of the given Profile attributes. " +
                "Skin outputs have one branch per attribute; " +
                "edge and outline outputs have one branch per attribute and skin.")
        {
        }

        protected override string ResponseArrayKey => "profiles";

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Identifier of each profile attribute.");

            OutIntegers(
                "Indices",
                "Index of each profile attribute.");

            OutTexts(
                "Names",
                "Name of each profile attribute.");

            OutBooleans(
                "WallType",
                "True if the profile is usable for walls.");

            OutBooleans(
                "BeamType",
                "True if the profile is usable for beams.");

            OutBooleans(
                "ColuType",
                "True if the profile is usable for columns.");

            OutBooleans(
                "HandrailType",
                "True if the profile is usable for handrails.");

            OutBooleans(
                "OtherGDLObjectType",
                "True if the profile is usable for other GDL objects.");

            OutTextTree(
                "UseWith",
                "Element types the profile can be used with (one branch per attribute).");

            OutNumberList(
                "Widths",
                "Width of each profile.");

            OutNumberList(
                "Heights",
                "Height of each profile.");

            OutNumberList(
                "MinimumWidths",
                "Minimum width of each profile.");

            OutNumberList(
                "MinimumHeights",
                "Minimum height of each profile.");

            OutBooleans(
                "WidthStretchable",
                "True if the profile width is stretchable.");

            OutBooleans(
                "HeightStretchable",
                "True if the profile height is stretchable.");

            OutBooleans(
                "HasCoreSkin",
                "True if the profile has a core skin.");

            OutTextTree(
                "ModifierNames",
                "Name of each profile modifier (one branch per attribute).");

            OutNumbers(
                "ModifierValues",
                "Value of each profile modifier (one branch per attribute).");

            OutTextTree(
                "SkinIds",
                "Identifier of each skin (one branch per attribute).");

            OutGenericTree(
                "SkinBuildingMaterialGuids",
                "Building material identifier of each skin (one branch per attribute).");

            OutGenericTree(
                "SkinSurfaceGuids",
                "Surface identifier of each skin (one branch per attribute).");

            OutGenericTree(
                "SkinFillGuids",
                "Fill identifier of each skin (one branch per attribute).");

            OutIntegerTree(
                "SkinContourPens",
                "Contour pen of each skin (one branch per attribute).");

            OutGenericTree(
                "SkinContourLineTypeGuids",
                "Contour line type identifier of each skin (one branch per attribute).");

            OutBooleanTree(
                "SkinIsCore",
                "True if the skin is a core skin (one branch per attribute).");

            OutBooleanTree(
                "SkinIsFinish",
                "True if the skin is a finish skin (one branch per attribute).");

            OutBooleanTree(
                "SkinVisibleCutEndLines",
                "True if the cut end lines of the skin are visible (one branch per attribute).");

            OutIntegerTree(
                "SkinCutEndLinePens",
                "Cut end line pen of each skin (one branch per attribute).");

            OutGenericTree(
                "SkinCutEndLineTypeGuids",
                "Cut end line type identifier of each skin (one branch per attribute).");

            OutGenericTree(
                "EdgeBuildingMaterialGuids",
                "Building material identifier of each skin edge (one branch per attribute and skin).");

            OutIntegerTree(
                "EdgePens",
                "Pen of each skin edge (one branch per attribute and skin).");

            OutGenericTree(
                "EdgeLineTypeGuids",
                "Line type identifier of each skin edge (one branch per attribute and skin).");

            OutBooleanTree(
                "EdgeIsVisibleLine",
                "True if the edge line is visible (one branch per attribute and skin).");

            OutBooleanTree(
                "EdgeIsCutEndLine",
                "True if the edge is a cut end line (one branch per attribute and skin).");

            OutBooleanTree(
                "EdgeIsInnerLine",
                "True if the edge is an inner line (one branch per attribute and skin).");

            OutPointTree(
                "OutlineCoords",
                "Outline points of each skin (one branch per attribute and skin).");

            OutIntegerTree(
                "OutlineSubPolyEnds",
                "Outline sub-polygon end indices of each skin (one branch per attribute and skin).");

            OutIntegerTree(
                "OutlineArcBegIndices",
                "Begin index of each outline arc (one branch per attribute and skin).");

            OutIntegerTree(
                "OutlineArcEndIndices",
                "End index of each outline arc (one branch per attribute and skin).");

            OutNumbers(
                "OutlineArcAngles",
                "Angle of each outline arc (one branch per attribute and skin).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried attribute (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var guids = new List<object>();
            var indices = new List<object>();
            var names = new List<object>();
            var wallType = new List<object>();
            var beamType = new List<object>();
            var coluType = new List<object>();
            var handrailType = new List<object>();
            var otherGDLObjectType = new List<object>();
            var useWith = new DataTree<object>();
            var widths = new List<object>();
            var heights = new List<object>();
            var minimumWidths = new List<object>();
            var minimumHeights = new List<object>();
            var widthStretchable = new List<object>();
            var heightStretchable = new List<object>();
            var hasCoreSkin = new List<object>();
            var modifierNames = new DataTree<object>();
            var modifierValues = new DataTree<object>();
            var skinIds = new DataTree<object>();
            var skinBuildingMaterialGuids = new DataTree<object>();
            var skinSurfaceGuids = new DataTree<object>();
            var skinFillGuids = new DataTree<object>();
            var skinContourPens = new DataTree<object>();
            var skinContourLineTypeGuids = new DataTree<object>();
            var skinIsCore = new DataTree<object>();
            var skinIsFinish = new DataTree<object>();
            var skinVisibleCutEndLines = new DataTree<object>();
            var skinCutEndLinePens = new DataTree<object>();
            var skinCutEndLineTypeGuids = new DataTree<object>();
            var edgeBuildingMaterialGuids = new DataTree<object>();
            var edgePens = new DataTree<object>();
            var edgeLineTypeGuids = new DataTree<object>();
            var edgeIsVisibleLine = new DataTree<object>();
            var edgeIsCutEndLine = new DataTree<object>();
            var edgeIsInnerLine = new DataTree<object>();
            var outlineCoords = new DataTree<object>();
            var outlineSubPolyEnds = new DataTree<object>();
            var outlineArcBegIndices = new DataTree<object>();
            var outlineArcEndIndices = new DataTree<object>();
            var outlineArcAngles = new DataTree<object>();
            var errors = new List<string>();

            var attributeTrees = new[]
            {
                useWith, modifierNames, modifierValues, skinIds,
                skinBuildingMaterialGuids, skinSurfaceGuids, skinFillGuids,
                skinContourPens, skinContourLineTypeGuids, skinIsCore,
                skinIsFinish, skinVisibleCutEndLines, skinCutEndLinePens,
                skinCutEndLineTypeGuids
            };

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                foreach (var tree in attributeTrees)
                {
                    tree.EnsurePath(path);
                }

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    guids.Add(null);
                    indices.Add(null);
                    names.Add(null);
                    wallType.Add(null);
                    beamType.Add(null);
                    coluType.Add(null);
                    handrailType.Add(null);
                    otherGDLObjectType.Add(null);
                    widths.Add(null);
                    heights.Add(null);
                    minimumWidths.Add(null);
                    minimumHeights.Add(null);
                    widthStretchable.Add(null);
                    heightStretchable.Add(null);
                    hasCoreSkin.Add(null);
                    continue;
                }

                errors.Add("");
                guids.Add(AttributeIdOf(item));
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                wallType.Add(JsonOutputHelp.Scalar(item, "wallType"));
                beamType.Add(JsonOutputHelp.Scalar(item, "beamType"));
                coluType.Add(JsonOutputHelp.Scalar(item, "coluType"));
                handrailType.Add(JsonOutputHelp.Scalar(item, "handrailType"));
                otherGDLObjectType.Add(JsonOutputHelp.Scalar(item, "otherGDLObjectType"));
                widths.Add(JsonOutputHelp.Scalar(item, "width"));
                heights.Add(JsonOutputHelp.Scalar(item, "height"));
                minimumWidths.Add(JsonOutputHelp.Scalar(item, "minimumWidth"));
                minimumHeights.Add(JsonOutputHelp.Scalar(item, "minimumHeight"));
                widthStretchable.Add(JsonOutputHelp.Scalar(item, "widthStretchable"));
                heightStretchable.Add(JsonOutputHelp.Scalar(item, "heightStretchable"));
                hasCoreSkin.Add(JsonOutputHelp.Scalar(item, "hasCoreSkin"));

                if (item["useWith"] is JArray useWithArray)
                {
                    foreach (var elementType in useWithArray)
                    {
                        useWith.Add(
                            elementType.ToString(),
                            path);
                    }
                }

                if (item["profileModifiers"] is JArray modifiers)
                {
                    foreach (var modifier in modifiers)
                    {
                        modifierNames.Add(
                            JsonOutputHelp.Scalar(modifier, "name"),
                            path);
                        modifierValues.Add(
                            JsonOutputHelp.Scalar(modifier, "value"),
                            path);
                    }
                }

                if (item["skins"] is JArray skins)
                {
                    for (var j = 0; j < skins.Count; j++)
                    {
                        var skin = skins[j];
                        skinIds.Add(
                            JsonOutputHelp.Scalar(skin, "skinId"),
                            path);
                        skinBuildingMaterialGuids.Add(
                            WrappedAttributeIdOf(skin["buildingMaterialId"]),
                            path);
                        skinSurfaceGuids.Add(
                            WrappedAttributeIdOf(skin["surfaceId"]),
                            path);
                        skinFillGuids.Add(
                            WrappedAttributeIdOf(skin["fillId"]),
                            path);
                        skinContourPens.Add(
                            JsonOutputHelp.Scalar(skin, "contourPen"),
                            path);
                        skinContourLineTypeGuids.Add(
                            WrappedAttributeIdOf(skin["contourLineTypeId"]),
                            path);
                        skinIsCore.Add(
                            JsonOutputHelp.Scalar(skin, "isCore"),
                            path);
                        skinIsFinish.Add(
                            JsonOutputHelp.Scalar(skin, "isFinish"),
                            path);
                        skinVisibleCutEndLines.Add(
                            JsonOutputHelp.Scalar(skin, "visibleCutEndLines"),
                            path);
                        skinCutEndLinePens.Add(
                            JsonOutputHelp.Scalar(skin, "cutEndLinePen"),
                            path);
                        skinCutEndLineTypeGuids.Add(
                            WrappedAttributeIdOf(skin["cutEndLineTypeId"]),
                            path);

                        var skinPath = new GH_Path(i, j);
                        edgeBuildingMaterialGuids.EnsurePath(skinPath);
                        edgePens.EnsurePath(skinPath);
                        edgeLineTypeGuids.EnsurePath(skinPath);
                        edgeIsVisibleLine.EnsurePath(skinPath);
                        edgeIsCutEndLine.EnsurePath(skinPath);
                        edgeIsInnerLine.EnsurePath(skinPath);
                        outlineCoords.EnsurePath(skinPath);
                        outlineSubPolyEnds.EnsurePath(skinPath);
                        outlineArcBegIndices.EnsurePath(skinPath);
                        outlineArcEndIndices.EnsurePath(skinPath);
                        outlineArcAngles.EnsurePath(skinPath);

                        if (skin["edges"] is JArray edges)
                        {
                            foreach (var edge in edges)
                            {
                                edgeBuildingMaterialGuids.Add(
                                    WrappedAttributeIdOf(edge["buildingMaterialId"]),
                                    skinPath);
                                edgePens.Add(
                                    JsonOutputHelp.Scalar(edge, "pen"),
                                    skinPath);
                                edgeLineTypeGuids.Add(
                                    WrappedAttributeIdOf(edge["lineTypeId"]),
                                    skinPath);
                                edgeIsVisibleLine.Add(
                                    JsonOutputHelp.Scalar(edge, "isVisibleLine"),
                                    skinPath);
                                edgeIsCutEndLine.Add(
                                    JsonOutputHelp.Scalar(edge, "isCutEndLine"),
                                    skinPath);
                                edgeIsInnerLine.Add(
                                    JsonOutputHelp.Scalar(edge, "isInnerLine"),
                                    skinPath);
                            }
                        }

                        if (skin["outlineCoords"] is JArray coords)
                        {
                            foreach (var coordinate in coords)
                            {
                                outlineCoords.Add(
                                    JsonOutputHelp.Point(coordinate),
                                    skinPath);
                            }
                        }

                        if (skin["outlineSubPolyEnds"] is JArray subPolyEnds)
                        {
                            foreach (var subPolyEnd in subPolyEnds)
                            {
                                outlineSubPolyEnds.Add(
                                    ((JValue)subPolyEnd).Value,
                                    skinPath);
                            }
                        }

                        if (skin["outlineArcs"] is JArray arcs)
                        {
                            foreach (var arc in arcs)
                            {
                                outlineArcBegIndices.Add(
                                    JsonOutputHelp.Scalar(arc, "begIndex"),
                                    skinPath);
                                outlineArcEndIndices.Add(
                                    JsonOutputHelp.Scalar(arc, "endIndex"),
                                    skinPath);
                                outlineArcAngles.Add(
                                    JsonOutputHelp.Scalar(arc, "arcAngle"),
                                    skinPath);
                            }
                        }
                    }
                }
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, indices);
            da.SetDataList(2, names);
            da.SetDataList(3, wallType);
            da.SetDataList(4, beamType);
            da.SetDataList(5, coluType);
            da.SetDataList(6, handrailType);
            da.SetDataList(7, otherGDLObjectType);
            da.SetDataTree(8, useWith);
            da.SetDataList(9, widths);
            da.SetDataList(10, heights);
            da.SetDataList(11, minimumWidths);
            da.SetDataList(12, minimumHeights);
            da.SetDataList(13, widthStretchable);
            da.SetDataList(14, heightStretchable);
            da.SetDataList(15, hasCoreSkin);
            da.SetDataTree(16, modifierNames);
            da.SetDataTree(17, modifierValues);
            da.SetDataTree(18, skinIds);
            da.SetDataTree(19, skinBuildingMaterialGuids);
            da.SetDataTree(20, skinSurfaceGuids);
            da.SetDataTree(21, skinFillGuids);
            da.SetDataTree(22, skinContourPens);
            da.SetDataTree(23, skinContourLineTypeGuids);
            da.SetDataTree(24, skinIsCore);
            da.SetDataTree(25, skinIsFinish);
            da.SetDataTree(26, skinVisibleCutEndLines);
            da.SetDataTree(27, skinCutEndLinePens);
            da.SetDataTree(28, skinCutEndLineTypeGuids);
            da.SetDataTree(29, edgeBuildingMaterialGuids);
            da.SetDataTree(30, edgePens);
            da.SetDataTree(31, edgeLineTypeGuids);
            da.SetDataTree(32, edgeIsVisibleLine);
            da.SetDataTree(33, edgeIsCutEndLine);
            da.SetDataTree(34, edgeIsInnerLine);
            da.SetDataTree(35, outlineCoords);
            da.SetDataTree(36, outlineSubPolyEnds);
            da.SetDataTree(37, outlineArcBegIndices);
            da.SetDataTree(38, outlineArcEndIndices);
            da.SetDataTree(39, outlineArcAngles);
            da.SetDataList(40, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProfiles;

        public override Guid ComponentGuid =>
            new Guid("601e3fbf-cf26-4a6f-a1cf-f45fa38bbac0");
    }
}
