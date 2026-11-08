using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetEditModeStateComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetEditModeState";

        // The editing modes the command reports, with their keys in the response.
        private static readonly string[] EditModeKeys =
        {
            "curtainWall",
            "stair",
            "beam",
            "column"
        };

        private static readonly string[] EditModeNames =
        {
            "CurtainWall",
            "Stair",
            "Beam",
            "Column"
        };

        public GetEditModeStateComponent()
            : base(
                "GetEditModeState",
                "Tells whether Archicad is in a hierarchical editing mode (curtain wall, stair, beam or column editing), " +
                "and which element is being edited. Read only: it never enters, leaves or cancels an editing mode. " +
                "Commands that modify the project can fail while such a mode is active. The overall InEditMode output " +
                "requires Archicad 29 or later; the command requires Archicad 27 or later.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "InEditMode",
                "True when Archicad is in any hierarchical editing mode. Only returned from Archicad 29.");

            OutTexts(
                "EditModes",
                "The editing modes reported: CurtainWall, Stair, Beam and Column.");

            OutBooleans(
                "IsActive",
                "True for each editing mode Archicad is in.");

            OutGenerics(
                "ElementGuids",
                "The element being edited in each editing mode (null when the mode is not active or Archicad does not report the element).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                (bool?)response["inEditMode"]);

            var isActive = new List<bool?>();
            var elementGuids = new List<ElementGuidWrapper>();

            foreach (var key in EditModeKeys)
            {
                var state = response[key];
                isActive.Add((bool?)state?["inEditMode"]);

                var guid = JsonOutputHelp.GuidOf(state?["elementId"]);
                elementGuids.Add(
                    guid == null
                        ? null
                        : new ElementGuidWrapper
                        {
                            ElementId = new ElementGuid { Guid = guid }
                        });
            }

            da.SetDataList(
                1,
                EditModeNames);

            da.SetDataList(
                2,
                isActive);

            da.SetDataList(
                3,
                elementGuids);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetEditModeState;

        public override Guid ComponentGuid =>
            new Guid("7dd2b283-fdf4-450b-bb47-2e57ef0b9970");
    }
}
