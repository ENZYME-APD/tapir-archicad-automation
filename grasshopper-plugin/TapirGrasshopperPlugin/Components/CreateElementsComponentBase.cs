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
            ElementGuid,
            AttributeGuid,
            PointsTree2D,
            PointsTree3D
        }

        protected sealed class Field
        {
            public Field(
                string inputName,
                string jsonKey,
                FieldKind kind,
                string description,
                bool required = false,
                int minPointsPerBranch = 0)
            {
                InputName = inputName;
                JsonKey = jsonKey;
                Kind = kind;
                Description = description;
                Required = required;
                MinPointsPerBranch = minPointsPerBranch;
            }

            public string InputName { get; }
            public string JsonKey { get; }
            public FieldKind Kind { get; }
            public string Description { get; }
            public bool Required { get; }
            public int MinPointsPerBranch { get; }
        }

        private readonly string _arrayKey;
        private readonly List<Field> _fields;

        // The first field must be required; it defines the number of created
        // elements (its list length, or its branch count for tree kinds).
        // Tree kinds are only supported as the first field.
        protected CreateElementsComponentBase(
            string name,
            string description,
            string subCategory,
            string arrayKey,
            List<Field> fields)
            : base(
                name,
                description,
                subCategory)
        {
            _arrayKey = arrayKey;
            _fields = fields;
        }

        protected override void AddInputs()
        {
            for (var index = 0; index < _fields.Count; index++)
            {
                var field = _fields[index];
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
                }

                if (!field.Required)
                {
                    SetOptionality(index);
                }
            }

            InTexts(
                "AdditionalSettings",
                "One JSON object per element with further optional settings matching the " +
                "command's documented item schema. Input only 1 to use the same settings for all. Optional.");
            SetOptionality(_fields.Count);
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

        protected override void Solve(
            IGH_DataAccess da)
        {
            var firstField = _fields[0];
            var isTreeFirst = firstField.Kind == FieldKind.PointsTree2D ||
                              firstField.Kind == FieldKind.PointsTree3D;

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
                    items.Add(new JObject
                    {
                        [firstField.JsonKey] = firstTokens[i]
                    });
                }
            }

            for (var fieldIndex = 1; fieldIndex < _fields.Count; fieldIndex++)
            {
                var field = _fields[fieldIndex];
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
                    items[i][field.JsonKey] = tokens.Count == 1
                        ? tokens[0].DeepClone()
                        : tokens[i];
                }
            }

            var additionalSettings = new List<string>();
            da.GetDataList(_fields.Count, additionalSettings);
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
            var parameters = new JObject { [_arrayKey] = itemsArray };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            SetCreatedElementsOutputs(da, response, 0, 1);
        }
    }
}
