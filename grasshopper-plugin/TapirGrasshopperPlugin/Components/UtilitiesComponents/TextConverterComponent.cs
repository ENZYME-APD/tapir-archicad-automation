using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class TextConverterComponent : Component
    {
        public TextConverterComponent()
            : base(
                "TextsToElements",
                "Converts Texts to Guid-based ElementObjects.",
                GroupNames.Utilities)
        {
        }

        protected override void AddInputs()
        {
            InTexts("ElementGuids");
        }

        protected override void AddOutputs()
        {
            OutGenerics(nameof(ElementGuid) + "s");
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> texts))
            {
                return;
            }

            da.SetDataList(
                0,
                texts.Select(x => ElementGuid.FromString(
                    x,
                    true)));
        }

        public override Guid ComponentGuid =>
            new Guid("c688fc6d-8bf2-4136-9d93-082915c72129");
    }
}