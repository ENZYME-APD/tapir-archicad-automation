using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class RemoveElementNotificationClientComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RemoveElementNotificationClient";

        public RemoveElementNotificationClientComponent()
            : base(
                "RemoveElementNotificationClient",
                "Removes an element notification client set up by SetElementNotificationClient.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Host",
                "The host address of the notification client. Defaults to localhost. Optional.");

            inManager.AddIntegerParameter(
                "Port",
                "Port",
                "The port number of the notification client."
                    .WithTypeName("integerItem"),
                GH_ParamAccess.item);

            SetOptionality(0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var port = 0;
            if (!da.GetData(1, ref port))
            {
                return;
            }

            var parameters = new JObject { ["port"] = port };

            string host = null;
            if (da.GetData(0, ref host) &&
                !string.IsNullOrEmpty(host))
            {
                parameters["host"] = host;
            }

            if (TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                this.CheckExecutionResult(response);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RemoveElementNotificationClient;

        public override Guid ComponentGuid =>
            new Guid("bc886b03-1ef1-458d-8b2d-73096f186567");
    }
}
