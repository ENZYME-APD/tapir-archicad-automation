using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class PublishPublisherSetComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PublishPublisherSet";

        public PublishPublisherSetComponent()
            : base(
                "PublishPublisherSet",
                "Perform a publish operation on the currently opened project. Only the given publisher set will be published.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "PublisherSetName",
                "The name of the publisher set to publish.");

            InText(
                "OutputPath",
                "Full local or LAN path for publishing. Optional; by default the publisher set's own path is used.");

            InGenerics(
                "SelectedNavigatorItemGuids",
                "Optional publisher-tree navigator items to publish instead of the whole set.");

            SetOptionality(new[] { 1, 2 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string publisherSetName) ||
                string.IsNullOrEmpty(publisherSetName))
            {
                this.AddError("PublisherSetName is required.");
                return;
            }

            var input = new PublishPublisherSetParameters
            {
                PublisherSetName = publisherSetName
            };

            if (da.TryGet(
                    1,
                    out string outputPath) &&
                !string.IsNullOrEmpty(outputPath))
            {
                input.OutputPath = outputPath;
            }

            if (da.TryGetList(
                    2,
                    out List<GH_ObjectWrapper> navWrappers) &&
                navWrappers.Count > 0)
            {
                input.SelectedNavigatorItemIds = new List<NavigatorGuidWrapper>();
                foreach (var wrapper in navWrappers)
                {
                    var id = GuidObject<NavigatorGuid>.CreateFromWrapper(wrapper);
                    if (id == null)
                    {
                        this.AddError("Invalid navigator item identifier.");
                        return;
                    }
                    input.SelectedNavigatorItemIds.Add(
                        new NavigatorGuidWrapper { NavigatorId = id });
                }
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PublishPublisherSet;

        public override Guid ComponentGuid =>
            new Guid("eda2a129-de0e-410f-b394-6c26af948a27");
    }
}
