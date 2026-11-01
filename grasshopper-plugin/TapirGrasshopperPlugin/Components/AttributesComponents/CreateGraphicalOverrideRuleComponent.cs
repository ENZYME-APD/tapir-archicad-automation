using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Drawing;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GraphicOverrides;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateGraphicalOverrideRuleComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateGraphicalOverrideRule";

        public CreateGraphicalOverrideRuleComponent()
            : base(
                "CreateGraphicalOverrideRule",
                "Creates or overwrites a graphic override rule in a rule group: name, criterion XML, surface color " +
                "and line pen. Needs Archicad 27 or later. CreateGraphicOverrideRules covers the whole style.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name of the rule. Rule names are unique in the project: an existing rule with this name is " +
                "overwritten, in whichever rule group it is.");

            InText(
                "RuleGroupName",
                "Rule group of a new rule, created when it does not exist. Optional, default: 'Tapir'. " +
                "An overwritten rule stays in its own group.");

            InText(
                "CriterionXML",
                "Criterion in Archicad's own XML format: read it from a rule made in Archicad with " +
                "GetGraphicalOverrideRules and change the values. XML written from scratch is dropped without an " +
                "error, and so is an empty one: the rule then applies to all elements. Optional.");

            InColor(
                "SurfaceColor",
                "Color override of the cut and uncut surfaces. Optional.");

            // No default values, so the keys are only sent when the inputs are given.
            inManager.AddIntegerParameter(
                "LinePenIndex",
                "LinePenIndex",
                "Pen index override for lines, markers and texts. Only a positive index takes effect. Optional."
                    .WithTypeName("integerItem"),
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "OverrideContours",
                "OverrideContours",
                "If true, the contours of the overridden surfaces are recolored too. Optional."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 1, 2, 3, 4, 5 });
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "RuleGuid",
                "Identifier of the created or overwritten rule.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string name))
            {
                return;
            }

            var parameters = new JObject { ["name"] = name };

            if (da.TryGet(
                    1,
                    out string ruleGroupName) &&
                !string.IsNullOrEmpty(ruleGroupName))
            {
                parameters["ruleGroupName"] = ruleGroupName;
            }

            if (da.TryGet(
                    2,
                    out string criterionXML) &&
                !string.IsNullOrEmpty(criterionXML))
            {
                parameters["criterionXML"] = criterionXML;
            }

            if (da.TryGet(
                    3,
                    out Color surfaceColor))
            {
                parameters["surfaceRGB"] = new JObject
                {
                    ["red"] = surfaceColor.R / 255.0,
                    ["green"] = surfaceColor.G / 255.0,
                    ["blue"] = surfaceColor.B / 255.0
                };
            }

            if (da.TryGet(
                    4,
                    out int linePenIndex))
            {
                parameters["linePenIndex"] = linePenIndex;
            }

            if (da.TryGet(
                    5,
                    out bool overrideContours))
            {
                parameters["overrideContours"] = overrideContours;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                GraphicOverrideHelp.RuleIdOf(response["ruleId"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateGraphicalOverrideRule;

        public override Guid ComponentGuid =>
            new Guid("f783dfb5-7844-4eea-aa1b-abf690c06cd4");
    }
}
