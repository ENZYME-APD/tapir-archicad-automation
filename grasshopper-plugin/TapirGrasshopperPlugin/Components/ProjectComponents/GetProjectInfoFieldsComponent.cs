using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetProjectInfoFieldsComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get project info.";
        public override string CommandName => "GetProjectInfoFields";

        public GetProjectInfoFieldsComponent()
            : base(
                "Project Info",
                "ProjectInfo",
                Doc,
                GroupNames.Project)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Project Info Id",
                "ProjectInfoId",
                "Project Info Id.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "Project Info Name",
                "ProjectInfoName",
                "Project Info Name.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "Project Info Value",
                "ProjectInfoValue",
                "Project Info Value.",
                GH_ParamAccess.list);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!GetConvertedResponse(
                    CommandName,
                    out ProjectInfoFields response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Fields.Select(x => x.ProjectInfoId));

            da.SetDataList(
                1,
                response.Fields.Select(x => x.ProjectInfoName));

            da.SetDataList(
                2,
                response.Fields.Select(x => x.ProjectInfoValue));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ProjectInfo;

        public override Guid ComponentGuid =>
            new Guid("bd3881e2-8c87-4220-898e-91f3fd984fd9");
    }
}