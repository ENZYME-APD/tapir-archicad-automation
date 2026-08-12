using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class ConnectMEPElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ConnectMEPElements";

        public ConnectMEPElementsComponent()
            : base(
                "ConnectMEPElements",
                "Connect MEP routing elements to other MEP elements or routes. " +
                "Merges routes, splits routes or creates branch elements as needed. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "RoutingElementGuids",
                "Identifiers of the MEP routing elements to connect.");

            InGenerics(
                "ConnectToGuids",
                "Identifier of the MEP element or route to connect each routing element to " +
                "(input only 1 to connect all routes to the same element).");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "DeletedRoutingElementGuids",
                "Identifier of the routing element deleted by merging, for each connection.");

            OutGenerics(
                "SplitRoutingElementGuids",
                "Identifier of the routing element created by splitting, for each connection.");

            OutGenerics(
                "CreatedBranchGuids",
                "Identifier of the branch element created by the connection, for each connection.");

            OutErrorMessages(
                "Error message of each connection (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> routingWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> connectToWrappers))
            {
                return;
            }

            if (connectToWrappers.Count != 1 &&
                connectToWrappers.Count != routingWrappers.Count)
            {
                this.AddError(
                    "The size of the input ConnectToGuids must be 1 or equal to the size of the input RoutingElementGuids.");
                return;
            }

            var connectToIds = new List<ElementGuid>();
            foreach (var wrapper in connectToWrappers)
            {
                var id = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid element identifier in the ConnectToGuids input.");
                    return;
                }
                connectToIds.Add(id);
            }

            var items = new JArray();
            for (var i = 0; i < routingWrappers.Count; i++)
            {
                var routingId = GuidObject<ElementGuid>.CreateFromWrapper(routingWrappers[i]);
                if (routingId == null)
                {
                    this.AddError("Invalid element identifier in the RoutingElementGuids input.");
                    return;
                }

                items.Add(
                    new JObject
                    {
                        ["routingElementId"] = new JObject { ["guid"] = routingId.Guid },
                        ["connectToId"] = new JObject
                        {
                            ["guid"] = connectToIds[connectToIds.Count == 1 ? 0 : i].Guid
                        }
                    });
            }

            var parameters = new JObject { ["connectionsData"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            // Each connection answers with the ids of the elements it changed:
            // any of the three can be missing when the connection did not need
            // that operation, so they are kept in separate outputs.
            var deletedIds = new List<ElementGuid>();
            var splitIds = new List<ElementGuid>();
            var branchIds = new List<ElementGuid>();
            var errorMessages = new List<string>();

            if (response["connectionResults"] is JArray results)
            {
                foreach (var result in results)
                {
                    var error = result?["error"];
                    errorMessages.Add(
                        error == null
                            ? ""
                            : error["message"]?.ToString() ?? "Unknown error.");

                    deletedIds.Add(
                        result?["deletedRoutingElementId"]?.ToObject<ElementGuid>());
                    splitIds.Add(
                        result?["splitRoutingElementId"]?.ToObject<ElementGuid>());
                    branchIds.Add(
                        result?["createdBranchId"]?.ToObject<ElementGuid>());
                }
            }

            da.SetDataList(0, deletedIds);
            da.SetDataList(1, splitIds);
            da.SetDataList(2, branchIds);
            da.SetDataList(3, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ConnectMEPElements;

        public override Guid ComponentGuid =>
            new Guid("6607fac7-4102-4409-9db4-51e567b0dbdd");
    }
}
