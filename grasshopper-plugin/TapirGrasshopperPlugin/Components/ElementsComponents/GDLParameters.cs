using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GDLParameters : Component
    {
        public GDLParameters()
            : base(
                "GDLParameters",
                "GDL parameter holder object.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InText(nameof(GdlParameterDetails.Name));
            InText(nameof(GdlParameterDetails.Index));
            InText(nameof(GdlParameterDetails.Type));
            InInteger(nameof(GdlParameterDetails.Dimension1));
            InInteger(nameof(GdlParameterDetails.Dimension2));
            InGeneric(nameof(GdlParameterDetails.Value));
        }

        protected override void AddOutputs()
        {
            OutText("JSON" + nameof(GdlParameterDetails));
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            var gdl = new GdlParameterDetails();

            if (!da.TryGet(
                    0,
                    out gdl.Name))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out gdl.Index))
            {
                return;
            }

            if (!da.TryGet(
                    2,
                    out gdl.Type))
            {
                return;
            }

            if (!da.TryGet(
                    3,
                    out gdl.Dimension1))
            {
                return;
            }

            if (!da.TryGet(
                    4,
                    out gdl.Dimension2))
            {
                return;
            }

            if (!da.TryGet(
                    5,
                    out gdl.Value))
            {
                return;
            }

            da.SetData(
                0,
                JObject.FromObject(gdl));
        }

        public override Guid ComponentGuid =>
            new Guid("87d9f273-33b5-4002-90fe-e3ca0101320a");
    }
}