using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class RefreshElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RefreshElements";

        public RefreshElementsComponent()
            : base(
                "RefreshElements",
                "Refreshes (rebuilds) the given elements, for example placed Objects, Windows and Doors that " +
                "still show the old geometry after their library part's script or embedded file changed. Set " +
                "Force to true to run the parameter script of GDL-based elements too. The elements are looked up " +
                "in the current database (floor plan, section or detail window). The refresh cannot be undone.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to refresh.");

            inManager.AddBooleanParameter(
                "Force",
                "Force",
                "Forces the rebuild of GDL-based elements, which runs their parameter script. Defaults to false. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it was refreshed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            var elements = new JArray();
            foreach (var elementId in elementIds)
            {
                elements.Add(new JObject { ["elementId"] = elementId });
            }

            var parameters = new JObject { ["elements"] = elements };

            var force = false;
            if (da.GetData(1, ref force))
            {
                parameters["force"] = force;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RefreshElements;

        public override Guid ComponentGuid =>
            new Guid("798438c6-0d10-427d-837a-3ae9207293a4");
    }
}
