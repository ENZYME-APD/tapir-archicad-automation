using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetIFCExportTranslatorsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetIFCExportTranslators";

        public GetIFCExportTranslatorsComponent()
            : base(
                "GetIFCExportTranslators",
                "Get the IFC export translators of the project, the preview " +
                "translator first. A translator name can be wired into the " +
                "TranslatorName input of IFCFileOperation.",
                GroupNames.IFC)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "TranslatorNames",
                "Names of the IFC export translators of the project, the " +
                "preview translator first.");

            OutText(
                "PreviewTranslatorName",
                "Name of the preview translator.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<GetIFCExportTranslatorsResponse>,
                    out GetIFCExportTranslatorsResponse response))
            {
                return;
            }

            if (response.Translators == null)
            {
                this.AddError("The response contains no translator list.");
                return;
            }

            da.SetDataList(
                0,
                response.Translators.Select(translator => translator.Name));

            var previewTranslator =
                response.Translators.FirstOrDefault(
                    translator => translator.Preview) ??
                response.Translators.FirstOrDefault();

            da.SetData(1, previewTranslator?.Name);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetIFCExportTranslators;

        public override Guid ComponentGuid =>
            new Guid("b7c9d3e8-2f5a-4e61-8d0f-9a4b6c2d7e13");
    }
}
