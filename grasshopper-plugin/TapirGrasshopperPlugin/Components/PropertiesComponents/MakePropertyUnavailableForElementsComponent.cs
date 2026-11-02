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
    public class MakePropertyUnavailableForElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MakePropertyUnavailableForElements";

        public MakePropertyUnavailableForElementsComponent()
            : base(
                "MakePropertyUnavailableForElements",
                "Make a property definition unavailable for all the given elements. " +
                "Available only in Archicad 27.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "The elements to make the property unavailable for.");

            InGeneric(
                "PropertyGuid",
                "The property definition to make unavailable.");
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
            Properties.Resources.MakePropertyUnavailableForElements;

        public override Guid ComponentGuid =>
            new Guid("48bb4bfa-e26a-4341-ab40-d29546c5e660");
    }
}
