using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // The column (primary) and row (secondary) patterns are given by their
    // pattern unit sizes as a tree - one branch per curtain wall, or a single
    // branch (e.g. a plain list) for all of them - plus their logic and origin.
    // The cell assignments are parallel trees: in the branch of a curtain wall
    // the k-th items of CellColumnIndices, CellRowIndices and the class inputs
    // together describe its k-th cell. A class input may hold 1 item per branch
    // (used for every cell of that branch) or nothing (the cells keep their
    // default class), and a null item leaves that class of that cell as it is.
    // The rarely used cell fields (rightPanelClassIndex, crossing frames) and
    // the flexible indices of the patterns go through AdditionalSettings.
    public class CreateCurtainWallsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateCurtainWalls";

        public CreateCurtainWallsComponent()
            : base(
                "CreateCurtainWalls",
                "Create Curtain Wall elements along reference polylines with a column (primary) and row " +
                "(secondary) pattern. Everything not given - the frame and panel classes, the surfaces, the " +
                "boundary and corner frames, the pattern - comes from the favorite or from the Curtain Wall " +
                "tool defaults; the pattern cells can assign the panel and frame classes to the grid.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "curtainWallsData";

        private const string CellsKey = "cells";

        private static readonly string[] PatternKeys = { "primaryPattern", "secondaryPattern" };

        private static readonly string[] CellClassKeys =
        {
            "panelClassIndex",
            "leftFrameClassIndex",
            "bottomFrameClassIndex"
        };

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("ReferenceLines", "coordinates", FieldKind.PointsTree2D, "Points of the reference polyline of each curtain wall on the floor plan (one branch per curtain wall, at least 2 points; only X and Y are used). Each pair of consecutive points becomes one segment.", required: true, minPointsPerBranch: 2),
            new Field("ZCoordinates", "zCoordinate", FieldKind.Number, "Z coordinate (absolute elevation) of the bottom of the curtain wall. When not given, the base level of the favorite or of the tool defaults relative to the floor is kept."),
            new Field("Heights", "height", FieldKind.Number, "Height of the curtain wall. When not given, the height of the favorite or of the tool defaults is kept."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the curtain wall. When not given, derived from ZCoordinates if that is given, otherwise the current story."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them."),
            new Field("PrimaryPatternSizes", "primaryPattern.sizes", FieldKind.NumbersTree, "Widths of the columns of the column (primary) pattern unit in meters, from the start of the segment; the unit repeats along the segment. One branch per curtain wall, or a single branch (a plain list) for all of them. When not given, the column pattern of the favorite or of the tool defaults is kept."),
            new Field("PrimaryPatternLogics", "primaryPattern.logic", FieldKind.Text, "How the column pattern fills the segment: FixedSizes (the remainder is a partial unit at the end) or BestDivision (the flexible columns are stretched so whole units fill the segment). Used with PrimaryPatternSizes; FixedSizes by default."),
            new Field("PrimaryPatternOrigins", "primaryPattern.origin", FieldKind.Text, "Where the column pattern starts: StartWithPattern, StartFromCenter, AlignToCenter or EndWithPattern. Used with PrimaryPatternSizes."),
            new Field("SecondaryPatternSizes", "secondaryPattern.sizes", FieldKind.NumbersTree, "Heights of the rows of the row (secondary) pattern unit in meters, from the bottom; the unit repeats up to the height. One branch per curtain wall, or a single branch (a plain list) for all of them. When not given, the row pattern of the favorite or of the tool defaults is kept."),
            new Field("SecondaryPatternLogics", "secondaryPattern.logic", FieldKind.Text, "How the row pattern fills the height: FixedSizes or BestDivision. Used with SecondaryPatternSizes; FixedSizes by default."),
            new Field("SecondaryPatternOrigins", "secondaryPattern.origin", FieldKind.Text, "Where the row pattern starts: StartWithPattern, StartFromCenter, AlignToCenter or EndWithPattern. Used with SecondaryPatternSizes."),
            new Field("CellColumnIndices", "cells.columnIndex", FieldKind.IntegersTree, "Zero-based column (from the start of the segment) of each cell of the pattern unit to assign classes to. One branch of cells per curtain wall, or a single branch for all of them; parallel with CellRowIndices and the cell class inputs. An assignment applies to every repetition of the cell."),
            new Field("CellRowIndices", "cells.rowIndex", FieldKind.IntegersTree, "Zero-based row (from the bottom) of each cell, parallel with CellColumnIndices."),
            new Field("CellPanelClasses", "cells.panelClassIndex", FieldKind.IntegersTree, "Panel class of each cell, parallel with CellColumnIndices (or 1 item per branch for all its cells): 0 is no panel, 1 the first panel class of the curtain wall (as listed in the Curtain Wall Settings), 2 the second and so on."),
            new Field("CellLeftFrameClasses", "cells.leftFrameClassIndex", FieldKind.IntegersTree, "Class of the frame on the left edge of each cell, parallel with CellColumnIndices (or 1 item per branch for all its cells): 0 no frame (merged with the neighbour), 1 Division, 2 Corner, 3 Boundary, 4 the first custom frame class and so on."),
            new Field("CellBottomFrameClasses", "cells.bottomFrameClassIndex", FieldKind.IntegersTree, "Class of the frame on the bottom edge of each cell, parallel with CellColumnIndices (or 1 item per branch for all its cells), numbered as CellLeftFrameClasses.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override string CompleteItem(
            JObject item)
        {
            foreach (var patternKey in PatternKeys)
            {
                if (!(item[patternKey] is JObject pattern))
                {
                    continue;
                }

                var inputPrefix = patternKey == "primaryPattern" ? "PrimaryPattern" : "SecondaryPattern";
                if (!(pattern["sizes"] is JArray sizes))
                {
                    return $"The inputs {inputPrefix}Logics and {inputPrefix}Origins need {inputPrefix}Sizes.";
                }

                if (sizes.Count == 0 ||
                    sizes.Any(size => size.Type == JTokenType.Null))
                {
                    return $"Every branch of the input {inputPrefix}Sizes must hold at least one size and no nulls.";
                }
            }

            if (!(item[CellsKey] is JObject cellColumns))
            {
                return null;
            }

            // The cell inputs arrive as one array per field; turn them into one
            // object per cell.
            if (!(cellColumns["columnIndex"] is JArray columnIndices) ||
                !(cellColumns["rowIndex"] is JArray rowIndices))
            {
                return "The cell inputs need both CellColumnIndices and CellRowIndices.";
            }

            if (columnIndices.Count != rowIndices.Count)
            {
                return "Each branch of CellRowIndices must have as many items as the branch of CellColumnIndices.";
            }

            var cells = new JArray();
            for (var cellIndex = 0; cellIndex < columnIndices.Count; cellIndex++)
            {
                if (columnIndices[cellIndex].Type == JTokenType.Null ||
                    rowIndices[cellIndex].Type == JTokenType.Null)
                {
                    return "CellColumnIndices and CellRowIndices must not contain nulls.";
                }

                cells.Add(new JObject
                {
                    ["columnIndex"] = columnIndices[cellIndex],
                    ["rowIndex"] = rowIndices[cellIndex]
                });
            }

            foreach (var classKey in CellClassKeys)
            {
                if (!(cellColumns[classKey] is JArray classIndices) ||
                    classIndices.Count == 0)
                {
                    continue;
                }

                if (classIndices.Count != 1 &&
                    classIndices.Count != cells.Count)
                {
                    return "Each branch of the cell class inputs must have 1 item or as many items as the branch of CellColumnIndices.";
                }

                for (var cellIndex = 0; cellIndex < cells.Count; cellIndex++)
                {
                    var classIndex = classIndices[classIndices.Count == 1 ? 0 : cellIndex];
                    if (classIndex.Type != JTokenType.Null)
                    {
                        cells[cellIndex][classKey] = classIndex.DeepClone();
                    }
                }
            }

            item[CellsKey] = cells;
            return null;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateCurtainWalls;

        public override Guid ComponentGuid =>
            new Guid("df74b361-90ee-4512-97a0-a92d6b9f296f");
    }
}
