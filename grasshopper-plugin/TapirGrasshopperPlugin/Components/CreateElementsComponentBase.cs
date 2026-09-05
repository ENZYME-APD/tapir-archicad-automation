using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components
{
    // Shared base for the Create* element commands. The commonly used fields
    // are separate typed inputs; the first (required) input defines the number
    // of created elements, every other input accepts a single value (applied
    // to every element) or one value per element. The rarely used or deeply
    // nested fields remain available through the optional AdditionalSettings
    // JSON input. The identifiers of the created elements are returned in the
    // ElementGuids output.
    //
    // The configuration is provided through overridable members instead of
    // constructor parameters on purpose: GH_Component's constructor calls
    // RegisterInputParams (and thus AddInputs) before the derived
    // constructor bodies run, so constructor-assigned fields would still be
    // null at that point. Overrides must not depend on instance state
    // (return constants or static data).
    public abstract class CreateElementsComponentBase : ArchicadExecutorComponent
    {
        protected enum FieldKind
        {
            Number,
            Integer,
            Boolean,
            Text,
            Point2D,
            Point3D,
            // A line writes begCoordinate, endCoordinate and zCoordinate (the
            // start point's Z) into the item; its JsonKey is unused.
            Line,
            ElementGuid,
            AttributeGuid,
            PointsTree2D,
            PointsTree3D,
            // A closed curve per element, written as polygonCoordinates plus the
            // polygonArcs of its arc segments. Its JsonKey names the coordinate
            // field; the arcs go next to it under polygonArcs.
            OutlineCurve,
            // One branch of closed curves per element, written as the element's
            // holes - each hole an object of polygonCoordinates and polygonArcs.
            HoleCurvesTree
        }

        protected sealed class Field
        {
            public Field(
                string inputName,
                string jsonKey,
                FieldKind kind,
                string description,
                bool required = false,
                int minPointsPerBranch = 0,
                Func<ValueList> valueList = null)
            {
                InputName = inputName;
                JsonKey = jsonKey;
                Kind = kind;
                Description = description;
                Required = required;
                MinPointsPerBranch = minPointsPerBranch;
                ValueList = valueList;
            }

            public string InputName { get; }
            public string JsonKey { get; }
            public FieldKind Kind { get; }
            public string Description { get; }
            public bool Required { get; }
            public int MinPointsPerBranch { get; }
            // Creates the value list offering the accepted options of a string
            // valued input; attached to the input automatically on the canvas.
            public Func<ValueList> ValueList { get; }
        }

        // The name of the command's array parameter (e.g. "wallsData").
        protected abstract string ArrayKey { get; }

        // The typed fields of the command. The first field must be required;
        // it defines the number of created elements (its list length, or its
        // branch count for tree kinds). Tree kinds are only supported as the
        // first field.
        protected abstract IReadOnlyList<Field> Fields { get; }

        // Override with false when the typed inputs cover the command's
        // complete item schema.
        protected virtual bool HasAdditionalSettingsInput => true;

        // Override with false when the command does not create new elements
        // the Tapir GH metadata could be embedded into (e.g. it modifies
        // existing ones).
        protected virtual bool SupportsElementMetadata => true;

        protected CreateElementsComponentBase(
            string name,
            string description,
            string subCategory)
            : base(
                name,
                description,
                subCategory)
        {
        }

        protected override void AddInputs()
        {
            var fields = Fields;
            for (var index = 0; index < fields.Count; index++)
            {
                var field = fields[index];
                var description = field.Description;
                if (index > 0)
                {
                    description += field.Required
                        ? " Input only 1 to use the same value for all elements."
                        : " Input only 1 to use the same value for all elements. Optional.";
                }

                switch (field.Kind)
                {
                    case FieldKind.Number:
                        InNumbers(field.InputName, description);
                        break;
                    case FieldKind.Integer:
                        InIntegers(field.InputName, description);
                        break;
                    case FieldKind.Boolean:
                        InBooleans(field.InputName, description);
                        break;
                    case FieldKind.Text:
                        InTexts(field.InputName, description);
                        break;
                    case FieldKind.Point2D:
                    case FieldKind.Point3D:
                        InPoints(field.InputName, description);
                        break;
                    case FieldKind.Line:
                        inManager.AddLineParameter(
                            field.InputName,
                            field.InputName,
                            description,
                            GH_ParamAccess.list);
                        break;
                    case FieldKind.ElementGuid:
                    case FieldKind.AttributeGuid:
                        InGenerics(field.InputName, description);
                        break;
                    case FieldKind.PointsTree2D:
                    case FieldKind.PointsTree3D:
                        inManager.AddPointParameter(
                            field.InputName,
                            field.InputName,
                            description,
                            GH_ParamAccess.tree);
                        break;
                    case FieldKind.OutlineCurve:
                        inManager.AddCurveParameter(
                            field.InputName,
                            field.InputName,
                            description,
                            GH_ParamAccess.list);
                        break;
                    case FieldKind.HoleCurvesTree:
                        inManager.AddCurveParameter(
                            field.InputName,
                            field.InputName,
                            description,
                            GH_ParamAccess.tree);
                        break;
                }

                if (!field.Required)
                {
                    SetOptionality(index);
                }
            }

            if (HasAdditionalSettingsInput)
            {
                InTexts(
                    "AdditionalSettings",
                    "One JSON object per element with further optional settings matching the " +
                    "command's documented item schema. Input only 1 to use the same settings for all. Optional.");
                SetOptionality(fields.Count);
            }

            if (SupportsElementMetadata)
            {
                InBoolean(
                    ElementMetadata.EmbedMetadataInputName,
                    ElementMetadata.EmbedMetadataDescription,
                    true);
                InBoolean(
                    ElementMetadata.ReplaceExistingInputName,
                    ElementMetadata.ReplaceExistingDescription,
                    false);
            }
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            var fields = Fields;
            for (var i = 0; i < fields.Count; i++)
            {
                fields[i].ValueList?.Invoke ().AddAsSource(this, i);
            }
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifiers of the created elements (null for failed items).");

            OutTexts(
                "ErrorMessages",
                "Error message for each item (empty when the element was created successfully).");
        }

        internal static void SetCreatedElementsOutputs(
            IGH_DataAccess da,
            JObject response,
            int elementGuidsIndex,
            int errorMessagesIndex)
        {
            var elementGuids = new List<object>();
            var errors = new List<string>();

            if (response?["elements"] is JArray items)
            {
                foreach (var item in items)
                {
                    if (item?["error"] != null)
                    {
                        errors.Add(item["error"]?["message"]?.ToString() ?? "");
                        elementGuids.Add(null);
                        continue;
                    }

                    errors.Add("");
                    var guid = item?["elementId"]?["guid"]?.ToString();
                    elementGuids.Add(
                        guid == null
                            ? null
                            : new ElementGuidWrapper
                            {
                                ElementId = new ElementGuid { Guid = guid }
                            });
                }
            }

            da.SetDataList(elementGuidsIndex, elementGuids);
            da.SetDataList(errorMessagesIndex, errors);
        }

        private static JArray ConvertBranchToCoordinates(
            IEnumerable<GH_Point> branch,
            bool is3D)
        {
            var coordinates = new JArray();
            foreach (var ghPoint in branch)
            {
                var coordinate = new JObject
                {
                    ["x"] = ghPoint.Value.X,
                    ["y"] = ghPoint.Value.Y
                };
                if (is3D)
                {
                    coordinate["z"] = ghPoint.Value.Z;
                }
                coordinates.Add(coordinate);
            }
            return coordinates;
        }

        // Turns a closed planar curve into the polygon Archicad expects: the node
        // coordinates, plus one polygonArcs entry per arc segment.
        //
        // begIndex/endIndex are 0 based within this contour's polygonCoordinates,
        // and the contour is closed implicitly - the add-on appends the first node
        // again itself, so the closing segment runs from the last index back to 0.
        //
        // arcAngle is positive when the arc bulges to the right of the straight
        // segment from beg to end. A Rhino arc whose plane normal points up (+Z) in
        // the world XY plane turns counter clockwise, which bulges to the left, so
        // its angle is negated.
        private static bool TryConvertCurveToPolygon(
            Curve curve,
            out JArray coordinates,
            out JArray arcs,
            out string error)
        {
            coordinates = new JArray();
            arcs = new JArray();
            error = null;

            if (curve == null)
            {
                error = "a curve is null";
                return false;
            }

            if (!curve.IsClosed)
            {
                error = "every outline curve has to be closed";
                return false;
            }

            var segments = curve is PolyCurve polyCurve
                ? polyCurve.DuplicateSegments()
                : new[] { curve };

            if (segments == null || segments.Length == 0)
            {
                error = "a curve has no segments";
                return false;
            }

            // A single closed segment has no corners to read - a circle, an ellipse,
            // a nurbs loop, or a polyline curve that is one object - so it is
            // approximated with a polyline. A closed arc in particular cannot be
            // expressed as one polygon arc: that would need a begIndex and an
            // endIndex that are the same node.
            if (segments.Length == 1 && segments[0].IsClosed)
            {
                var polyline = TessellateToPolyline(segments[0]);
                if (polyline == null)
                {
                    error = "a curve could not be approximated with a polyline";
                    return false;
                }
                // The polyline of a closed curve repeats its first point at the end;
                // the add-on closes the contour itself, so that repeat is dropped.
                for (var i = 0; i < polyline.Count - 1; i++)
                {
                    coordinates.Add(new JObject
                    {
                        ["x"] = polyline[i].X,
                        ["y"] = polyline[i].Y
                    });
                }
                return coordinates.Count >= 3;
            }

            foreach (var segment in segments)
            {
                var start = segment.PointAtStart;
                var nodeIndex = coordinates.Count;
                coordinates.Add(new JObject
                {
                    ["x"] = start.X,
                    ["y"] = start.Y
                });

                if (segment.IsLinear())
                {
                    continue;
                }

                if (segment.TryGetArc(out Rhino.Geometry.Arc arc))
                {
                    var angle = arc.Angle;
                    if (arc.Plane.Normal.Z > 0.0)
                    {
                        angle = -angle;
                    }
                    arcs.Add(new JObject
                    {
                        ["begIndex"] = nodeIndex,
                        // The last segment closes back onto the first node.
                        ["endIndex"] = nodeIndex + 1,
                        ["arcAngle"] = angle
                    });
                    continue;
                }

                var tessellated = TessellateToPolyline(segment);
                if (tessellated == null)
                {
                    error = "a curve segment could not be approximated with a polyline";
                    return false;
                }
                // The segment's own start is already in, and its end is the next
                // segment's start, so only the points in between are added.
                for (var i = 1; i < tessellated.Count - 1; i++)
                {
                    coordinates.Add(new JObject
                    {
                        ["x"] = tessellated[i].X,
                        ["y"] = tessellated[i].Y
                    });
                }
            }

            if (coordinates.Count < 3)
            {
                error = "an outline needs at least three points";
                return false;
            }

            // Fix up the closing arc: its end node is 0, not the count.
            foreach (var arcToken in arcs)
            {
                if ((int)arcToken["endIndex"] >= coordinates.Count)
                {
                    arcToken["endIndex"] = 0;
                }
            }

            return true;
        }

        private static Polyline TessellateToPolyline(
            Curve curve)
        {
            var polylineCurve = curve.ToPolyline(
                0,
                0,
                0.05,
                0.0,
                0.0,
                0.01,
                0.0,
                0.0,
                true);
            return polylineCurve?.ToPolyline();
        }

        private static JToken ConvertGuidWrapper<T>(
            GH_ObjectWrapper wrapper)
            where T : GuidObject<T>, new()
        {
            var id = GuidObject<T>.CreateFromWrapper(wrapper);
            return id == null
                ? null
                : new JObject { ["guid"] = id.Guid };
        }

        private static List<JToken> ConvertValues<T>(
            List<T> values,
            Func<T, JToken> convert)
        {
            var tokens = new List<JToken>();
            foreach (var value in values)
            {
                var token = convert(value);
                if (token == null)
                {
                    return null;
                }
                tokens.Add(token);
            }
            return tokens;
        }

        // Reads the values of a non-tree field input. Returns false on invalid
        // values; an unconnected optional input yields an empty token list.
        private bool TryReadTokens(
            IGH_DataAccess da,
            int inputIndex,
            Field field,
            out List<JToken> tokens)
        {
            switch (field.Kind)
            {
                case FieldKind.OutlineCurve:
                    {
                        var curves = new List<Curve>();
                        da.GetDataList(inputIndex, curves);
                        tokens = new List<JToken>();
                        foreach (var curve in curves)
                        {
                            if (!TryConvertCurveToPolygon(curve, out JArray coordinates, out JArray arcs, out string error))
                            {
                                this.AddError($"The input {field.InputName} is invalid: {error}.");
                                tokens = null;
                                return false;
                            }

                            var outline = new JObject { [field.JsonKey] = coordinates };
                            if (arcs.Count > 0)
                            {
                                outline["polygonArcs"] = arcs;
                            }
                            tokens.Add(outline);
                        }
                        break;
                    }
                case FieldKind.HoleCurvesTree:
                    {
                        tokens = new List<JToken>();
                        if (!da.TryGetTree(inputIndex, out GH_Structure<GH_Curve> holeTree))
                        {
                            break;
                        }
                        foreach (var branch in holeTree.Branches)
                        {
                            var holes = new JArray();
                            foreach (var ghCurve in branch)
                            {
                                if (ghCurve == null)
                                {
                                    continue;
                                }
                                if (!TryConvertCurveToPolygon(ghCurve.Value, out JArray coordinates, out JArray arcs, out string error))
                                {
                                    this.AddError($"The input {field.InputName} is invalid: {error}.");
                                    tokens = null;
                                    return false;
                                }

                                var hole = new JObject { ["polygonCoordinates"] = coordinates };
                                if (arcs.Count > 0)
                                {
                                    hole["polygonArcs"] = arcs;
                                }
                                holes.Add(hole);
                            }
                            tokens.Add(holes);
                        }
                        break;
                    }
                case FieldKind.Number:
                    {
                        var values = new List<double>();
                        da.GetDataList(inputIndex, values);
                        tokens = ConvertValues(values, v => (JToken)v);
                        break;
                    }
                case FieldKind.Integer:
                    {
                        var values = new List<int>();
                        da.GetDataList(inputIndex, values);
                        tokens = ConvertValues(values, v => (JToken)v);
                        break;
                    }
                case FieldKind.Boolean:
                    {
                        var values = new List<bool>();
                        da.GetDataList(inputIndex, values);
                        tokens = ConvertValues(values, v => (JToken)v);
                        break;
                    }
                case FieldKind.Text:
                    {
                        var values = new List<string>();
                        da.GetDataList(inputIndex, values);
                        tokens = ConvertValues(values, v => (JToken)v);
                        break;
                    }
                case FieldKind.Point2D:
                    {
                        var values = new List<Point3d>();
                        da.GetDataList(inputIndex, values);
                        tokens = ConvertValues(values, v =>
                            (JToken)new JObject { ["x"] = v.X, ["y"] = v.Y });
                        break;
                    }
                case FieldKind.Point3D:
                    {
                        var values = new List<Point3d>();
                        da.GetDataList(inputIndex, values);
                        tokens = ConvertValues(values, v =>
                            (JToken)new JObject { ["x"] = v.X, ["y"] = v.Y, ["z"] = v.Z });
                        break;
                    }
                case FieldKind.Line:
                    {
                        var values = new List<Line>();
                        da.GetDataList(inputIndex, values);
                        tokens = ConvertValues(values, v =>
                            (JToken)new JObject
                            {
                                ["begCoordinate"] = new JObject { ["x"] = v.From.X, ["y"] = v.From.Y },
                                ["endCoordinate"] = new JObject { ["x"] = v.To.X, ["y"] = v.To.Y },
                                ["zCoordinate"] = v.From.Z
                            });
                        break;
                    }
                case FieldKind.ElementGuid:
                    {
                        var values = new List<GH_ObjectWrapper>();
                        da.GetDataList(inputIndex, values);
                        tokens = ConvertValues(values, ConvertGuidWrapper<ElementGuid>);
                        break;
                    }
                case FieldKind.AttributeGuid:
                    {
                        var values = new List<GH_ObjectWrapper>();
                        da.GetDataList(inputIndex, values);
                        tokens = ConvertValues(values, ConvertGuidWrapper<AttributeGuidObject>);
                        break;
                    }
                default:
                    this.AddError(
                        $"Tree inputs are only supported as the first input ({field.InputName}).");
                    tokens = null;
                    return false;
            }

            if (tokens == null)
            {
                this.AddError(
                    $"Invalid value in the {field.InputName} input.");
                return false;
            }

            return true;
        }

        private static void AssignToken(
            JObject item,
            Field field,
            JToken token)
        {
            if (field.Kind == FieldKind.Line || field.Kind == FieldKind.OutlineCurve)
            {
                foreach (var property in ((JObject)token).Properties())
                {
                    item[property.Name] = property.Value.DeepClone();
                }
            }
            else
            {
                item[field.JsonKey] = token;
            }
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var fields = Fields;
            var firstField = fields[0];
            var isTreeFirst = firstField.Kind == FieldKind.PointsTree2D ||
                              firstField.Kind == FieldKind.PointsTree3D;
            // OutlineCurve is read as a plain list, so it needs no special casing
            // here - TryReadTokens turns each curve into the item's polygon.

            int itemCount;
            var items = new List<JObject>();
            if (isTreeFirst)
            {
                if (!da.TryGetTree(0, out GH_Structure<GH_Point> tree))
                {
                    return;
                }
                itemCount = tree.Branches.Count;
                if (itemCount == 0)
                {
                    this.AddError(
                        $"The input {firstField.InputName} must contain at least one branch.");
                    return;
                }

                var is3D = firstField.Kind == FieldKind.PointsTree3D;
                for (var i = 0; i < itemCount; i++)
                {
                    var branch = tree.Branches[i];
                    if (branch.Count < firstField.MinPointsPerBranch)
                    {
                        this.AddError(
                            $"Each branch of the {firstField.InputName} input must contain at least {firstField.MinPointsPerBranch} points.");
                        return;
                    }
                    items.Add(new JObject
                    {
                        [firstField.JsonKey] = ConvertBranchToCoordinates(branch, is3D)
                    });
                }
            }
            else
            {
                if (!TryReadTokens(da, 0, firstField, out List<JToken> firstTokens))
                {
                    return;
                }
                itemCount = firstTokens.Count;
                if (itemCount == 0)
                {
                    this.AddError(
                        $"The input {firstField.InputName} must contain at least one item.");
                    return;
                }

                for (var i = 0; i < itemCount; i++)
                {
                    var item = new JObject();
                    AssignToken(item, firstField, firstTokens[i]);
                    items.Add(item);
                }
            }

            for (var fieldIndex = 1; fieldIndex < fields.Count; fieldIndex++)
            {
                var field = fields[fieldIndex];
                if (!TryReadTokens(da, fieldIndex, field, out List<JToken> tokens))
                {
                    return;
                }

                if (tokens.Count == 0)
                {
                    if (field.Required)
                    {
                        this.AddError(
                            $"The required input {field.InputName} is empty.");
                        return;
                    }
                    continue;
                }

                if (tokens.Count != 1 && tokens.Count != itemCount)
                {
                    this.AddError(
                        $"The size of the input {field.InputName} must be 1 or equal to the size of the input {firstField.InputName}.");
                    return;
                }

                for (var i = 0; i < itemCount; i++)
                {
                    AssignToken(
                        items[i],
                        field,
                        tokens.Count == 1 ? tokens[0].DeepClone() : tokens[i]);
                }
            }

            var additionalSettings = new List<string>();
            if (HasAdditionalSettingsInput)
            {
                da.GetDataList(fields.Count, additionalSettings);
            }
            if (additionalSettings.Count > 0)
            {
                if (additionalSettings.Count != 1 &&
                    additionalSettings.Count != itemCount)
                {
                    this.AddError(
                        "The size of the input AdditionalSettings must be 0, 1 or equal to the size of the first input.");
                    return;
                }

                for (var i = 0; i < itemCount; i++)
                {
                    var json = additionalSettings[additionalSettings.Count == 1 ? 0 : i];
                    try
                    {
                        items[i].Merge(
                            JObject.Parse(json),
                            new JsonMergeSettings
                            {
                                MergeArrayHandling = MergeArrayHandling.Replace
                            });
                    }
                    catch (Exception ex)
                    {
                        this.AddError(
                            $"Invalid JSON in the AdditionalSettings input: {ex.Message}");
                        return;
                    }
                }
            }

            var itemsArray = new JArray();
            foreach (var item in items)
            {
                itemsArray.Add(item);
            }
            var parameters = new JObject { [ArrayKey] = itemsArray };

            var embedMetadata = false;
            var replaceExisting = false;
            if (SupportsElementMetadata)
            {
                var metadataInputIndex =
                    fields.Count + (HasAdditionalSettingsInput ? 1 : 0);
                embedMetadata = da.GetOptional(metadataInputIndex, true);
                replaceExisting = da.GetOptional(metadataInputIndex + 1, false);
            }

            var metadata = new ElementMetadata(this, ToAddOn, ToArchicad);
            // The previous elements are collected before the creation (so the
            // new elements are never in the set), but only deleted after it
            // succeeded, so a failed run does not lose them.
            JArray previousElements = null;
            if (replaceExisting)
            {
                previousElements = metadata.FindPreviouslyCreatedElements();
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            if (embedMetadata)
            {
                metadata.StampCreatedElements(response);
            }
            if (replaceExisting)
            {
                metadata.DeletePreviouslyCreatedElements(previousElements);
            }

            SetCreatedElementsOutputs(da, response, 0, 1);
        }
    }
}
