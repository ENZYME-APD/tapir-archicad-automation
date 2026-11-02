using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    // The fields of the Project Notes (File > Info > Project Notes): the
    // parameter name of the components, the key of the commands' JSON and
    // the description.
    internal static class ProjectNotesFields
    {
        public static readonly string[,] Fields =
        {
            { "Client", "client", "Client." },
            { "ProjectName", "projectName", "Project name." },
            { "Company", "company", "Company." },
            { "Street", "street", "Street." },
            { "City", "city", "City." },
            { "Country", "country", "Country." },
            { "PostalCode", "postalCode", "Postal code." },
            { "Architect", "architect", "Architect." },
            { "Draftsmen", "draftsmen", "Draftsmen." },
            { "ProjectStatus", "projectStatus", "Project status." },
            { "DateOfIssue", "dateOfIssue", "Date of issue (free text, as typed in Project Notes)." },
            { "Keywords", "keywords", "Keywords." },
            { "Notes", "notes", "Notes." }
        };

        public static int Count => Fields.GetLength(0);

        public static string Name(
            int index) => Fields[index, 0];

        public static string Key(
            int index) => Fields[index, 1];

        public static string Description(
            int index) => Fields[index, 2];
    }

    public class GetProjectNotesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetProjectNotes";

        public GetProjectNotesComponent()
            : base(
                "GetProjectNotes",
                "Gets the Project Notes of the project (File > Info > Project Notes): client, project name, company, " +
                "address, architect, draftsmen, project status, date of issue, keywords and notes.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            for (var i = 0; i < ProjectNotesFields.Count; i++)
            {
                OutText(
                    ProjectNotesFields.Name(i),
                    ProjectNotesFields.Description(i));
            }
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

            for (var i = 0; i < ProjectNotesFields.Count; i++)
            {
                da.SetData(
                    i,
                    response[ProjectNotesFields.Key(i)]?.ToString());
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProjectNotes;

        public override Guid ComponentGuid =>
            new Guid("29c19953-0f34-44f2-b67a-1533f70c477a");
    }
}
