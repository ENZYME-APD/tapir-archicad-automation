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
    public class MakePropertyAvailableForElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MakePropertyAvailableForElements";

        public MakePropertyAvailableForElementsComponent()
            : base(
                "MakePropertyAvailableForElements",
                "Make a property definition available for all the given elements. " +
                "Available only in Archicad 27.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "The elements to make the property available for.");

            InGeneric(
                "PropertyGuid",
                "The property definition to make available.");
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

            var parameters = new JObject
            {
                ["elements"] = elementIds,
                ["propertyId"] = new JObject { ["guid"] = propertyId.Guid }
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
            Properties.Resources.MakePropertyAvailableForElements;

        public override Guid ComponentGuid =>
            new Guid("ba2ca777-5fdc-4a1a-a282-915d8f77f6f2");
    }
}
