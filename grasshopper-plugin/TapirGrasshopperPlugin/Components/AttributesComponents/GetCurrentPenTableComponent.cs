using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetCurrentPenTableComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCurrentPenTable";

        public GetCurrentPenTableComponent()
            : base(
                "GetCurrentPenTable",
                "Returns the current Pen Table (its index, attribute identifier and name) and its number of pens. " +
                "Pass the returned AttributeGuid to GetPenTables to read the pens.",
                GroupNames.Attributes)
        {
        }

        protected override void AddOutputs()
        {
            OutInteger(
                "Index",
                "The attribute index of the current Pen Table, as returned by Archicad.");

            OutBoolean(
                "IsPenTableAttribute",
                "True if the index refers to an existing Pen Table attribute. When false, AttributeGuid and Name " +
                "are null; the pens in effect can not be read with GetPenTables.");

            OutGeneric(
                "AttributeGuid",
                "Identifier of the current Pen Table attribute.");

            OutText(
                "Name",
                "Name of the current Pen Table attribute.");

            OutInteger(
                "PenCount",
                "The number of pens in the current Pen Table. Null when it can not be read.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                JsonOutputHelp.Scalar(response, "index"));

            da.SetData(
                1,
                JsonOutputHelp.Scalar(response, "isPenTableAttribute"));

            var guid = JsonOutputHelp.WrappedGuidOf(
                response,
                "attributeId");
            da.SetData(
                2,
                guid == null
                    ? null
                    : new AttributeGuidWrapper
                    {
                        AttributeId = new AttributeGuidObject { Guid = guid }
                    });

            da.SetData(
                3,
                JsonOutputHelp.Scalar(response, "name"));

            da.SetData(
                4,
                JsonOutputHelp.Scalar(response, "penCount"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCurrentPenTable;

        public override Guid ComponentGuid =>
            new Guid("895a3064-4a4c-4d36-a671-2d4f78d206a5");
    }
}
