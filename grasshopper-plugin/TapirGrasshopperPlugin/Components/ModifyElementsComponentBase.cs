using Grasshopper.Kernel;
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
    // Shared base for the Modify* element commands. The element ids and the
    // commonly used detail fields are separate typed inputs; each field input
    // accepts a single value (applied to every element) or one value per
    // element. The rarely used or deeply nested fields remain available
    // through the optional AdditionalSettings JSON input.
    public abstract class ModifyElementsComponentBase : ArchicadExecutorComponent
    {
        protected enum FieldKind
        {
            Number,
            Integer,
            Boolean,
            Text,
            Point2D,
            Point3D,
            AttributeGuid
        }

        protected sealed class Field
        {
            public Field(
                string inputName,
                string jsonKey,
                FieldKind kind,
                string description)
            {
                InputName = inputName;
                JsonKey = jsonKey;
                Kind = kind;
                Description = description;
            }

            public string InputName { get; }
            public string JsonKey { get; }
            public FieldKind Kind { get; }
            public string Description { get; }
        }

        private readonly string _arrayKey;
        private readonly string _itemWrapKey;
        private readonly List<Field> _fields;

        protected ModifyElementsComponentBase(
            string name,
            string description,
            string subCategory,
            string arrayKey,
            List<Field> fields,
            string itemWrapKey = null)
            : base(
                name,
                description,
                subCategory)
        {
            _arrayKey = arrayKey;
            _itemWrapKey = itemWrapKey;
            _fields = fields;
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to modify.");

            var index = 1;
            foreach (var field in _fields)
            {
                var description = field.Description +
                    " Input only 1 to use the same value for all elements. Optional.";
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
                    case FieldKind.AttributeGuid:
                        InGenerics(field.InputName, description);
                        break;
                }

                SetOptionality(index);
                index++;
            }

            InTexts(
                "AdditionalSettings",
                "One JSON object per element with further optional settings matching the " +
                "command's documented item schema. Input only 1 to use the same settings for all. Optional.");
            SetOptionality(index);
        }

        private bool TryApply<T>(
            IGH_DataAccess da,
            int inputIndex,
            string inputName,
            IReadOnlyList<JObject> targets,
            Func<T, JToken> convert)
        {
            var values = new List<T>();
            da.GetDataList(inputIndex, values);

            if (values.Count == 0)
            {
                return true;
            }

            if (values.Count != 1 && values.Count != targets.Count)
            {
                this.AddError(
                    $"The size of the input {inputName} must be 0, 1 or equal to the size of the input ElementGuids.");
                return false;
            }

            var field = _fields[inputIndex - 1];
            for (var i = 0; i < targets.Count; i++)
            {
                var token = convert(values[values.Count == 1 ? 0 : i]);
                if (token == null)
                {
                    this.AddError(
                        $"Invalid value in the {inputName} input.");
                    return false;
                }
                targets[i][field.JsonKey] = token;
            }

            return true;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            var items = new JArray();
            var targets = new List<JObject>();
            foreach (var element in elements.Elements)
            {
                var item = JObject.FromObject(element);
                if (_itemWrapKey != null)
                {
                    var wrapped = new JObject();
                    item[_itemWrapKey] = wrapped;
                    targets.Add(wrapped);
                }
                else
                {
                    targets.Add(item);
                }
                items.Add(item);
            }

            for (var fieldIndex = 0; fieldIndex < _fields.Count; fieldIndex++)
            {
                var field = _fields[fieldIndex];
                var inputIndex = fieldIndex + 1;
                bool ok;
                switch (field.Kind)
                {
                    case FieldKind.Number:
                        ok = TryApply<double>(da, inputIndex, field.InputName, targets, v => v);
                        break;
                    case FieldKind.Integer:
                        ok = TryApply<int>(da, inputIndex, field.InputName, targets, v => v);
                        break;
                    case FieldKind.Boolean:
                        ok = TryApply<bool>(da, inputIndex, field.InputName, targets, v => v);
                        break;
                    case FieldKind.Text:
                        ok = TryApply<string>(da, inputIndex, field.InputName, targets, v => v);
                        break;
                    case FieldKind.Point2D:
                        ok = TryApply<Point3d>(da, inputIndex, field.InputName, targets, v =>
                            new JObject { ["x"] = v.X, ["y"] = v.Y });
                        break;
                    case FieldKind.Point3D:
                        ok = TryApply<Point3d>(da, inputIndex, field.InputName, targets, v =>
                            new JObject { ["x"] = v.X, ["y"] = v.Y, ["z"] = v.Z });
                        break;
                    case FieldKind.AttributeGuid:
                        ok = TryApply<GH_ObjectWrapper>(da, inputIndex, field.InputName, targets, v =>
                        {
                            var id = GuidObject<AttributeGuidObject>.CreateFromWrapper(v);
                            return id == null
                                ? null
                                : new JObject { ["guid"] = id.Guid };
                        });
                        break;
                    default:
                        ok = true;
                        break;
                }

                if (!ok)
                {
                    return;
                }
            }

            var additionalSettings = new List<string>();
            da.GetDataList(_fields.Count + 1, additionalSettings);
            if (additionalSettings.Count > 0)
            {
                if (additionalSettings.Count != 1 &&
                    additionalSettings.Count != targets.Count)
                {
                    this.AddError(
                        "The size of the input AdditionalSettings must be 0, 1 or equal to the size of the input ElementGuids.");
                    return;
                }

                for (var i = 0; i < targets.Count; i++)
                {
                    var json = additionalSettings[additionalSettings.Count == 1 ? 0 : i];
                    try
                    {
                        targets[i].Merge(
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

            var parameters = new JObject { [_arrayKey] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }
    }
}
