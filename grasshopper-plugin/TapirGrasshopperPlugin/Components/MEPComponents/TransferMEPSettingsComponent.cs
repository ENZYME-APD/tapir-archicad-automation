using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class TransferMEPSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "TransferMEPSettings";

        public TransferMEPSettingsComponent()
            : base(
                "TransferMEPSettings",
                "Picks up the settings of an MEP element and injects them into other MEP elements of the same type " +
                "and domain, like Archicad's pick up and inject parameters. Works for routing elements, terminals, " +
                "accessories, equipment, fittings, flexible segments, branches, take-offs and (from Archicad 30) " +
                "electrical panels and devices. Each target is changed in its own undoable step. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "SourceElementGuid",
                "The MEP element to pick up the settings from.");

            InGenerics(
                "TargetElementGuids",
                "The MEP elements to inject the settings into. Each must be of the type and domain of the source element.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each target element (empty when its settings were changed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out GH_ObjectWrapper sourceWrapper))
            {
                return;
            }
            var sourceId = GuidObject<ElementGuid>.CreateFromWrapper(sourceWrapper);
            if (sourceId == null)
            {
                this.AddError("Invalid identifier in the SourceElementGuid input.");
                return;
            }

            if (!da.TryCreateFromList(
                    1,
                    out ElementsObject targets))
            {
                return;
            }

            var parameters = new JObject
            {
                ["sourceElementId"] = new JObject { ["guid"] = sourceId.Guid },
                ["targetElements"] = JObject.FromObject(targets)["elements"]
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMEPPorts;

        public override Guid ComponentGuid =>
            new Guid("166d842a-dd25-476b-ab3d-3aa865e657d2");
    }
}
