using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetSurveyPointTransformationComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetSurveyPointTransformation";

        public GetSurveyPointTransformationComponent()
            : base(
                "GetSurveyPointTransformation",
                "Gets the transformation of points from the Project Origin coordinate system into the Survey Point " +
                "coordinate system, and its inverse when it is invertible. Read only. Available from Archicad 27.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            outManager.AddTransformParameter(
                "ProjectToSurvey",
                "ProjectToSurvey",
                "Transforms a point given in the Project Origin coordinate system into the Survey Point coordinate system (in meters).".WithTypeName("transformItem"),
                GH_ParamAccess.item);

            outManager.AddTransformParameter(
                "SurveyToProject",
                "SurveyToProject",
                ("Transforms a point given in the Survey Point coordinate system into the Project Origin coordinate " +
                 "system (in meters). Null when ProjectToSurvey is not invertible.").WithTypeName("transformItem"),
                GH_ParamAccess.item);

            OutNumberList(
                "ProjectToSurveyMatrix",
                "The 3x4 matrix of ProjectToSurvey row by row (12 numbers), as Archicad returns it.");

            OutNumberList(
                "SurveyToProjectMatrix",
                "The 3x4 matrix of SurveyToProject row by row (12 numbers); empty when it is missing.");
        }

        // A row by row 3x4 matrix as a transform; null when it is not 12 numbers.
        private static Transform? ToTransform(
            List<double> matrix)
        {
            if (matrix == null ||
                matrix.Count != 12)
            {
                return null;
            }

            var transform = Transform.Identity;
            for (var row = 0; row < 3; row++)
            {
                for (var column = 0; column < 4; column++)
                {
                    transform[row, column] = matrix[row * 4 + column];
                }
            }

            return transform;
        }

        private static List<double> MatrixOf(
            JToken token)
        {
            return token is JArray array
                ? array.Select(x => x.Value<double>()).ToList()
                : null;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var projectToSurvey = MatrixOf(response["projectToSurvey"]);
            var surveyToProject = MatrixOf(response["surveyToProject"]);

            var projectToSurveyTransform = ToTransform(projectToSurvey);
            if (projectToSurveyTransform.HasValue)
            {
                da.SetData(0, projectToSurveyTransform.Value);
            }

            var surveyToProjectTransform = ToTransform(surveyToProject);
            if (surveyToProjectTransform.HasValue)
            {
                da.SetData(1, surveyToProjectTransform.Value);
            }

            da.SetDataList(2, projectToSurvey ?? new List<double>());
            da.SetDataList(3, surveyToProject ?? new List<double>());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetSurveyPointTransformation;

        public override Guid ComponentGuid =>
            new Guid("7e887bcb-08ec-427c-b1d8-7c0844f77fa4");
    }
}
