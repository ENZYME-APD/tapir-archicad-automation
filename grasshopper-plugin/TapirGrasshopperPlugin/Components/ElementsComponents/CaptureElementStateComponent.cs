using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // An executor, not an accessor: every capture stores a new state in the
    // add-on's memory, so it must not re-run on every solution.
    public class CaptureElementStateComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CaptureElementState";

        public CaptureElementStateComponent()
            : base(
                "CaptureElementState",
                "Capture the state of the elements of the project, so GetElementChangesSince can list " +
                "the elements created, modified and deleted since. The state is kept in the add-on's memory " +
                "only: it is dropped when the project is closed or Archicad quits, and ReleaseElementState " +
                "frees it earlier.",
                GroupNames.Elements)
        {
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementStateTypeValueList>(
                document,
                0);
        }

        protected override void AddInputs()
        {
            InTextWithDefault(
                "Type",
                "How the changes are found between this state and another one. ModificationStamp (the default) " +
                "compares the modification stamps of the elements, so any change counts. 3DModel compares the 3D " +
                "model of the elements, so only a change that alters the 3D model counts, and capturing it " +
                "generates the 3D model of the project. Context is the context based difference generator of the " +
                "Archicad API. Two states can only be compared when they were captured with the same type.",
                ElementStateTypes.ModificationStamp);

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "StateId",
                "Identifier of the captured state, for GetElementChangesSince and ReleaseElementState.");

            OutText(
                "Type",
                "The type the state was captured with.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var type = da.GetOptional(
                0,
                ElementStateTypes.ModificationStamp);

            var parameters = new CaptureElementStateParameters
            {
                Type = string.IsNullOrWhiteSpace(type)
                    ? null
                    : type
            };

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    JHelp.Deserialize<CaptureElementStateResponse>,
                    out CaptureElementStateResponse response))
            {
                return;
            }

            da.SetData(
                0,
                response.StateId);

            da.SetData(
                1,
                response.Type);
        }

        // No icon of its own yet: the state is a snapshot of all elements.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AllElems;

        public override Guid ComponentGuid =>
            new Guid("7c1e4a92-5b3d-4f8e-9a61-2d7c0b58e4f3");
    }
}
