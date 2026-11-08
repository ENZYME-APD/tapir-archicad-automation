using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class ActivateLayerCombinationComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ActivateLayerCombination";

        public ActivateLayerCombinationComponent()
            : base(
                "ActivateLayerCombination",
                "Activate a layer combination in the current Archicad window, like choosing it in the Quick Layers palette. Saved views are not changed.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "AttributeGuid",
                "Identifier of the layer combination to activate.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out GH_ObjectWrapper wrapper) ||
                wrapper?.Value == null)
            {
                this.AddError("AttributeGuid is required.");
                return;
            }

            var attributeId = GuidObject<AttributeGuidObject>.CreateFromWrapper(wrapper);
            if (attributeId == null)
            {
                this.AddError("Invalid AttributeGuid.");
                return;
            }

            var input = new ActivateLayerCombinationParameters
            {
                AttributeId = attributeId
            };

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
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
        }

        // TODO: add a dedicated ActivateLayerCombination.png resource; the
        // binary asset could not be added in this automated change.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLayerCombinations;

        public override Guid ComponentGuid =>
            new Guid("3f6d2c8a-9b41-4e7f-a5c2-7d18e0b4f6a9");
    }
}
