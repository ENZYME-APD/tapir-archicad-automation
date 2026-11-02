using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class ModifyPropertyValueOfElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyPropertyValueOfElements";

        public ModifyPropertyValueOfElementsComponent()
            : base(
                "ModifyPropertyValueOfElements",
                "Set one property value on all the given elements in a single step. If the property is not " +
                "available for any of the elements, none of them is changed. Available from Archicad 27.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "The elements to set the value on. The property must be available for every one of them.");

            InGeneric(
                "PropertyGuid",
                "The property to set.");

            InText(
                "Value",
                "The new value of the property, as its display string.");
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers))
            {
                return;
            }

            var elementIds = new JArray();
            foreach (var wrapper in elementWrappers)
            {
                var id = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return;
                }
                elementIds.Add(
                    new JObject
                    {
                        ["elementId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            if (!da.TryGet(
                    1,
                    out GH_ObjectWrapper propertyWrapper))
            {
                return;
            }

            var propertyId = GuidObject<PropertyGuidObject>.CreateFromWrapper(propertyWrapper);
            if (propertyId == null)
            {
                this.AddError("Invalid property identifier in the PropertyGuid input.");
                return;
            }

            if (!da.TryGet(
                    2,
                    out string value))
            {
                return;
            }

            var parameters = new JObject
            {
                ["elements"] = elementIds,
                ["propertyId"] = new JObject { ["guid"] = propertyId.Guid },
                ["propertyValue"] = new JObject { ["value"] = value }
            };

            // The command answers with one ExecutionResult: it succeeds or fails
            // as a whole, so there is nothing per item to hand out.
            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(0, response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyPropertyValueOfElements;

        public override Guid ComponentGuid =>
            new Guid("f10807b3-29ec-4f7e-ab66-c9dfdc507cdd");
    }
}
