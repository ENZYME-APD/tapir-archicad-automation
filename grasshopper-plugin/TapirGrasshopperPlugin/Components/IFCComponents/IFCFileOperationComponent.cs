using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class IFCFileOperationComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "IFCFileOperation";

        public IFCFileOperationComponent()
            : base(
                "IFCFileOperation",
                "Execute an IFC file operation (save, merge or open).",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Method",
                "The file operation method to use: save, merge or open.");

            InText(
                "IfcFilePath",
                "The target IFC file to use.");

            InTextWithDefault(
                "FileType",
                "The type of the IFC file: ifc, ifcxml, ifczip or ifcxmlzip.",
                "ifc");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string method) ||
                string.IsNullOrEmpty(method))
            {
                this.AddError("Method is required.");
                return;
            }

            if (!da.TryGet(
                    1,
                    out string ifcFilePath) ||
                string.IsNullOrEmpty(ifcFilePath))
            {
                this.AddError("IfcFilePath is required.");
                return;
            }

            da.TryGet(
                2,
                out string fileType);

            var input = new IFCFileOperationParameters
            {
                Method = method,
                IfcFilePath = ifcFilePath,
                FileType = string.IsNullOrEmpty(fileType) ? null : fileType
            };

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.IFCFileOperation;

        public override Guid ComponentGuid =>
            new Guid("64403220-e7e3-4eda-8e6a-33c0e79080a0");
    }
}
