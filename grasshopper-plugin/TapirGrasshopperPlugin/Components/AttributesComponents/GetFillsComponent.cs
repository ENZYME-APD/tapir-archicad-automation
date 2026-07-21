using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetFillsComponent : AttributeStructuredGetterComponent
    {
        public override string CommandName => "GetFills";

        public GetFillsComponent()
            : base(
                "GetFills",
                "Get the details of the given Fill attributes.")
        {
        }

        protected override string ResponseArrayKey => "fills";

        private static readonly string[] TextureBooleanFields =
        {
            "FillRectangle", "FitPicture", "mirrorX", "mirrorY",
            "useAlphaChannel", "alphaChannelChangesTransparency",
            "alphaChannelChangesSurfaceColor", "alphaChannelChangesAmbientColor",
            "alphaChannelChangesSpecularColor", "alphaChannelChangesDiffuseColor"
        };

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Identifier of each fill attribute.");

            OutIntegers(
                "Indices",
                "Index of each fill attribute.");

            OutTexts(
                "Names",
                "Name of each fill attribute.");

            OutTexts(
                "SubTypes",
                "Sub type of each fill: Vector, Solid, Empty, Symbol, LinearGradient or RadialGradient.");

            OutBooleans(
                "ScaleWithPlan",
                "True if the fill is scale independent.");

            OutBooleans(
                "UseForWalls",
                "True if the fill is usable for cut walls.");

            OutBooleans(
                "UseForDraft",
                "True if the fill is usable for drafting.");

            OutBooleans(
                "UseForCover",
                "True if the fill is usable as cover fill.");

            OutNumberList(
                "HorizontalSpacings",
                "Horizontal spacing of the fill pattern.");

            OutNumberList(
                "VerticalSpacings",
                "Vertical spacing of the fill pattern.");

            OutNumberList(
                "Angles",
                "Angle of the fill pattern.");

            OutTexts(
                "BitPatterns",
                "Bit pattern of each vector fill.");

            OutPoints(
                "GradientStarts",
                "Gradient start point of each gradient fill.");

            OutPoints(
                "GradientEnds",
                "Gradient end point of each gradient fill.");

            OutNumberList(
                "Percents",
                "Percent of each fill.");

            OutTexts(
                "TextureNames",
                "Name of the texture of each fill.");

            OutNumberList(
                "TextureRotationAngles",
                "Rotation angle of the texture of each fill.");

            OutNumberList(
                "TextureXSizes",
                "X size of the texture of each fill.");

            OutNumberList(
                "TextureYSizes",
                "Y size of the texture of each fill.");

            OutBooleans(
                "TextureFillRectangle",
                "True if the texture fills the rectangle.");

            OutBooleans(
                "TextureFitPicture",
                "True if the texture picture is fitted.");

            OutBooleans(
                "TextureMirrorX",
                "True if the texture is mirrored in X direction.");

            OutBooleans(
                "TextureMirrorY",
                "True if the texture is mirrored in Y direction.");

            OutBooleans(
                "TextureUseAlphaChannel",
                "True if the texture alpha channel is used.");

            OutBooleans(
                "TextureAlphaChangesTransparency",
                "True if the alpha channel changes the transparency.");

            OutBooleans(
                "TextureAlphaChangesSurfaceColor",
                "True if the alpha channel changes the surface color.");

            OutBooleans(
                "TextureAlphaChangesAmbientColor",
                "True if the alpha channel changes the ambient color.");

            OutBooleans(
                "TextureAlphaChangesSpecularColor",
                "True if the alpha channel changes the specular color.");

            OutBooleans(
                "TextureAlphaChangesDiffuseColor",
                "True if the alpha channel changes the diffuse color.");

            OutNumbers(
                "LineItemFrequencies",
                "Frequency of each vector fill line (one branch per attribute).");

            OutNumbers(
                "LineItemDirections",
                "Direction angle of each vector fill line (one branch per attribute).");

            OutNumbers(
                "LineItemOffsetLines",
                "Offset of each vector fill line (one branch per attribute).");

            OutPointTree(
                "LineItemOffsets",
                "Offset point of each vector fill line (one branch per attribute).");

            OutNumbers(
                "LineItemLineLengths",
                "Dash lengths of each vector fill line (one branch per attribute and line).");

            OutPointTree(
                "SymbolLineBegins",
                "Begin point of each symbol fill line (one branch per attribute).");

            OutPointTree(
                "SymbolLineEnds",
                "End point of each symbol fill line (one branch per attribute).");

            OutPointTree(
                "SymbolArcBegins",
                "Begin point of each symbol fill arc (one branch per attribute).");

            OutPointTree(
                "SymbolArcOrigins",
                "Origin point of each symbol fill arc (one branch per attribute).");

            OutNumbers(
                "SymbolArcAngles",
                "Angle of each symbol fill arc (one branch per attribute).");

            OutPointTree(
                "SymbolHotspots",
                "Hotspot points of each symbol fill (one branch per attribute).");

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
            var subTypes = new List<object>();
            var scaleWithPlan = new List<object>();
            var useForWalls = new List<object>();
            var useForDraft = new List<object>();
            var useForCover = new List<object>();
            var horizontalSpacings = new List<object>();
            var verticalSpacings = new List<object>();
            var angles = new List<object>();
            var bitPatterns = new List<object>();
            var gradientStarts = new List<object>();
            var gradientEnds = new List<object>();
            var percents = new List<object>();
            var textureNames = new List<object>();
            var textureRotationAngles = new List<object>();
            var textureXSizes = new List<object>();
            var textureYSizes = new List<object>();
            var textureBooleans = new List<object>[TextureBooleanFields.Length];
            for (var f = 0; f < TextureBooleanFields.Length; f++)
            {
                textureBooleans[f] = new List<object>();
            }
            var lineItemFrequencies = new DataTree<object>();
            var lineItemDirections = new DataTree<object>();
            var lineItemOffsetLines = new DataTree<object>();
            var lineItemOffsets = new DataTree<object>();
            var lineItemLineLengths = new DataTree<object>();
            var symbolLineBegins = new DataTree<object>();
            var symbolLineEnds = new DataTree<object>();
            var symbolArcBegins = new DataTree<object>();
            var symbolArcOrigins = new DataTree<object>();
            var symbolArcAngles = new DataTree<object>();
            var symbolHotspots = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                lineItemFrequencies.EnsurePath(path);
                lineItemDirections.EnsurePath(path);
                lineItemOffsetLines.EnsurePath(path);
                lineItemOffsets.EnsurePath(path);
                symbolLineBegins.EnsurePath(path);
                symbolLineEnds.EnsurePath(path);
                symbolArcBegins.EnsurePath(path);
                symbolArcOrigins.EnsurePath(path);
                symbolArcAngles.EnsurePath(path);
                symbolHotspots.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    guids.Add(null);
                    indices.Add(null);
                    names.Add(null);
                    subTypes.Add(null);
                    scaleWithPlan.Add(null);
                    useForWalls.Add(null);
                    useForDraft.Add(null);
                    useForCover.Add(null);
                    horizontalSpacings.Add(null);
                    verticalSpacings.Add(null);
                    angles.Add(null);
                    bitPatterns.Add(null);
                    gradientStarts.Add(null);
                    gradientEnds.Add(null);
                    percents.Add(null);
                    textureNames.Add(null);
                    textureRotationAngles.Add(null);
                    textureXSizes.Add(null);
                    textureYSizes.Add(null);
                    foreach (var list in textureBooleans)
                    {
                        list.Add(null);
                    }
                    continue;
                }

                errors.Add("");
                guids.Add(AttributeIdOf(item));
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                subTypes.Add(JsonOutputHelp.Scalar(item, "subType"));
                scaleWithPlan.Add(JsonOutputHelp.Scalar(item, "scaleWithPlan"));
                useForWalls.Add(JsonOutputHelp.Scalar(item, "useForWalls"));
                useForDraft.Add(JsonOutputHelp.Scalar(item, "useForDraft"));
                useForCover.Add(JsonOutputHelp.Scalar(item, "useForCover"));
                horizontalSpacings.Add(JsonOutputHelp.Scalar(item, "horizontalSpacing"));
                verticalSpacings.Add(JsonOutputHelp.Scalar(item, "verticalSpacing"));
                angles.Add(JsonOutputHelp.Scalar(item, "angle"));
                bitPatterns.Add(JsonOutputHelp.Scalar(item, "bitPattern"));
                gradientStarts.Add(JsonOutputHelp.Point(item["gradientStart"]));
                gradientEnds.Add(JsonOutputHelp.Point(item["gradientEnd"]));
                percents.Add(JsonOutputHelp.Scalar(item, "percent"));

                var texture = item["texture"];
                textureNames.Add(JsonOutputHelp.Scalar(texture, "name"));
                textureRotationAngles.Add(JsonOutputHelp.Scalar(texture, "rotationAngle"));
                textureXSizes.Add(JsonOutputHelp.Scalar(texture, "xSize"));
                textureYSizes.Add(JsonOutputHelp.Scalar(texture, "ySize"));
                for (var f = 0; f < TextureBooleanFields.Length; f++)
                {
                    textureBooleans[f].Add(
                        JsonOutputHelp.Scalar(texture, TextureBooleanFields[f]));
                }

                if (item["lineItems"] is JArray lineItems)
                {
                    for (var j = 0; j < lineItems.Count; j++)
                    {
                        var lineItem = lineItems[j];
                        lineItemFrequencies.Add(
                            JsonOutputHelp.Scalar(lineItem, "frequency"),
                            path);
                        lineItemDirections.Add(
                            JsonOutputHelp.Scalar(lineItem, "direction"),
                            path);
                        lineItemOffsetLines.Add(
                            JsonOutputHelp.Scalar(lineItem, "offsetLine"),
                            path);
                        lineItemOffsets.Add(
                            JsonOutputHelp.Point(lineItem["offset"]),
                            path);

                        var linePath = new GH_Path(i, j);
                        lineItemLineLengths.EnsurePath(linePath);
                        if (lineItem["lineLengths"] is JArray lineLengths)
                        {
                            foreach (var length in lineLengths)
                            {
                                lineItemLineLengths.Add(
                                    ((JValue)length).Value,
                                    linePath);
                            }
                        }
                    }
                }

                if (item["symbolLines"] is JArray symbolLines)
                {
                    foreach (var symbolLine in symbolLines)
                    {
                        symbolLineBegins.Add(
                            JsonOutputHelp.Point(symbolLine["begin"]),
                            path);
                        symbolLineEnds.Add(
                            JsonOutputHelp.Point(symbolLine["end"]),
                            path);
                    }
                }

                if (item["symbolArcs"] is JArray symbolArcs)
                {
                    foreach (var symbolArc in symbolArcs)
                    {
                        symbolArcBegins.Add(
                            JsonOutputHelp.Point(symbolArc["begin"]),
                            path);
                        symbolArcOrigins.Add(
                            JsonOutputHelp.Point(symbolArc["origin"]),
                            path);
                        symbolArcAngles.Add(
                            JsonOutputHelp.Scalar(symbolArc, "angle"),
                            path);
                    }
                }

                if (item["symbolHotspots"] is JArray hotspots)
                {
                    foreach (var hotspot in hotspots)
                    {
                        symbolHotspots.Add(
                            JsonOutputHelp.Point(hotspot),
                            path);
                    }
                }
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, indices);
            da.SetDataList(2, names);
            da.SetDataList(3, subTypes);
            da.SetDataList(4, scaleWithPlan);
            da.SetDataList(5, useForWalls);
            da.SetDataList(6, useForDraft);
            da.SetDataList(7, useForCover);
            da.SetDataList(8, horizontalSpacings);
            da.SetDataList(9, verticalSpacings);
            da.SetDataList(10, angles);
            da.SetDataList(11, bitPatterns);
            da.SetDataList(12, gradientStarts);
            da.SetDataList(13, gradientEnds);
            da.SetDataList(14, percents);
            da.SetDataList(15, textureNames);
            da.SetDataList(16, textureRotationAngles);
            da.SetDataList(17, textureXSizes);
            da.SetDataList(18, textureYSizes);
            for (var f = 0; f < TextureBooleanFields.Length; f++)
            {
                da.SetDataList(19 + f, textureBooleans[f]);
            }
            da.SetDataTree(29, lineItemFrequencies);
            da.SetDataTree(30, lineItemDirections);
            da.SetDataTree(31, lineItemOffsetLines);
            da.SetDataTree(32, lineItemOffsets);
            da.SetDataTree(33, lineItemLineLengths);
            da.SetDataTree(34, symbolLineBegins);
            da.SetDataTree(35, symbolLineEnds);
            da.SetDataTree(36, symbolArcBegins);
            da.SetDataTree(37, symbolArcOrigins);
            da.SetDataTree(38, symbolArcAngles);
            da.SetDataTree(39, symbolHotspots);
            da.SetDataList(40, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetFills;

        public override Guid ComponentGuid =>
            new Guid("37878650-64df-4df7-8818-d321a899956f");
    }
}
