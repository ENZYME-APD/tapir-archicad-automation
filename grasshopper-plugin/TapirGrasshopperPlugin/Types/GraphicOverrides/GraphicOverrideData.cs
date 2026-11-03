using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.GraphicOverrides
{
    public class GraphicOverrideRuleGuid : GuidObject<GraphicOverrideRuleGuid>
    {
    }

    public class GraphicOverrideRuleGuidWrapper
        : GuidWrapper<GraphicOverrideRuleGuid, GraphicOverrideRuleGuidWrapper>
    {
        [JsonProperty("graphicOverrideRuleId")]
        public GraphicOverrideRuleGuid GraphicOverrideRuleId;

        [JsonIgnore]
        public override GraphicOverrideRuleGuid Id
        {
            get => GraphicOverrideRuleId;
            set => GraphicOverrideRuleId = value;
        }
    }

    public class GraphicOverrideCombinationGuid
        : GuidObject<GraphicOverrideCombinationGuid>
    {
    }

    public class GraphicOverrideCombinationGuidWrapper
        : GuidWrapper<GraphicOverrideCombinationGuid, GraphicOverrideCombinationGuidWrapper>
    {
        [JsonProperty("graphicOverrideCombinationId")]
        public GraphicOverrideCombinationGuid GraphicOverrideCombinationId;

        [JsonIgnore]
        public override GraphicOverrideCombinationGuid Id
        {
            get => GraphicOverrideCombinationId;
            set => GraphicOverrideCombinationId = value;
        }
    }

    // Shared conversions of the graphic override components.
    public static class GraphicOverrideHelp
    {
        public static string StyleKeysDoc =>
            "Keys (all optional, a left out one is not overridden): lineType, fillType and surface " +
            "({\"guid\": ...} of an attribute), lineMarkerTextPen, fillForegroundPen and fillBackgroundPen " +
            "(pen index), fillBackgroundColor and surfaceColor ({\"red\", \"green\", \"blue\"} in 0..1), " +
            "fillTypeAppliesTo, fillForegroundPenAppliesTo and fillBackgroundPenAppliesTo " +
            "({\"cutFill\", \"coverFill\", \"draftingFill\"} booleans, left out: all fills), surfaceAppliesTo and " +
            "hiddenContours ({\"cutSurface\", \"uncutSurface\"} booleans), showSkinSeparators, " +
            "overridePenColorAndThickness and overrideContours (booleans).";

        public static string CriterionDoc =>
            "Structured criterion as JSON with exactly one key, e.g. {\"elementType\": \"Wall\"}, " +
            "{\"classification\": \"Slab\"}, {\"layer\": \"Walls\"} or {\"and\": [{...}, {...}]} " +
            "(see the GraphicOverrideCriterion schema of the Tapir documentation for every condition). " +
            "Use either this or the criterion XML for a rule, not both.";

        // Wrapper of a rule identifier object like {"guid": "..."}; null when absent.
        public static GraphicOverrideRuleGuidWrapper RuleIdOf(
            JToken idObject)
        {
            var guid = JsonOutputHelp.GuidOf(idObject);
            if (guid == null)
            {
                return null;
            }

            return new GraphicOverrideRuleGuidWrapper
            {
                GraphicOverrideRuleId = new GraphicOverrideRuleGuid { Guid = guid }
            };
        }

        // Wrapper of a combination identifier object like {"guid": "..."}; null when absent.
        public static GraphicOverrideCombinationGuidWrapper CombinationIdOf(
            JToken idObject)
        {
            var guid = JsonOutputHelp.GuidOf(idObject);
            if (guid == null)
            {
                return null;
            }

            return new GraphicOverrideCombinationGuidWrapper
            {
                GraphicOverrideCombinationId =
                    new GraphicOverrideCombinationGuid { Guid = guid }
            };
        }

        // The rule identifiers of an array of {"graphicOverrideRuleId": {"guid": ...}} items.
        public static List<GraphicOverrideRuleGuidWrapper> RuleIdsOf(
            JToken ruleIdItems)
        {
            var ids = new List<GraphicOverrideRuleGuidWrapper>();
            if (ruleIdItems is JArray items)
            {
                foreach (var item in items)
                {
                    ids.Add(RuleIdOf(item?["graphicOverrideRuleId"]));
                }
            }

            return ids;
        }

        // {"guid": ...} of a rule given as anything a GUID input accepts; null when invalid.
        public static JObject RuleIdObject(
            GH_ObjectWrapper wrapper)
        {
            var id = GuidObject<GraphicOverrideRuleGuid>.CreateFromWrapper(wrapper);
            return id == null
                ? null
                : new JObject { ["guid"] = id.Guid };
        }

        // {"guid": ...} of a combination given as anything a GUID input accepts; null when invalid.
        public static JObject CombinationIdObject(
            GH_ObjectWrapper wrapper)
        {
            var id = GuidObject<GraphicOverrideCombinationGuid>.CreateFromWrapper(wrapper);
            return id == null
                ? null
                : new JObject { ["guid"] = id.Guid };
        }

        // The rules of one branch of a tree input as [{"graphicOverrideRuleId": {"guid": ...}}];
        // null when an item is not a rule identifier.
        public static JArray RuleIdItemsOf(
            IEnumerable<IGH_Goo> branch)
        {
            var items = new JArray();
            foreach (var goo in branch)
            {
                if (goo == null)
                {
                    continue;
                }

                // A generic parameter hands its tree out as IGH_Goo (#634).
                var wrapper = goo as GH_ObjectWrapper ?? new GH_ObjectWrapper(goo);
                var id = RuleIdObject(wrapper);
                if (id == null)
                {
                    return null;
                }

                items.Add(new JObject { ["graphicOverrideRuleId"] = id });
            }

            return items;
        }

        // The branch of a tree input that belongs to the item at index: the only
        // branch when there is one, otherwise the branch at that index.
        public static List<IGH_Goo> BranchFor(
            GH_Structure<IGH_Goo> tree,
            int index)
        {
            if (tree == null ||
                tree.Branches.Count == 0)
            {
                return new List<IGH_Goo>();
            }

            return tree.Branches[tree.Branches.Count == 1 ? 0 : index];
        }

        // The value for the item at index of an input that holds 0, 1 (the same
        // for all) or one value per item; default when it holds none.
        public static T ValueFor<T>(
            List<T> values,
            int index)
        {
            if (values == null ||
                values.Count == 0)
            {
                return default;
            }

            return values[values.Count == 1 ? 0 : index];
        }

        // Whether an input holding count values may be used for itemCount items:
        // 0, 1 or itemCount values. Adds the error to the component when not.
        public static bool CheckCount(
            GH_ActiveObject component,
            string inputName,
            int count,
            string itemsInputName,
            int itemCount)
        {
            if (count <= 1 ||
                count == itemCount)
            {
                return true;
            }

            component.AddError(
                $"The size of the input {inputName} must be 0, 1 or equal to the size of the input {itemsInputName}.");
            return false;
        }

        // A JSON object input; null with an error added when the text is not a JSON object.
        public static bool TryParseObject(
            GH_ActiveObject component,
            string inputName,
            string text,
            out JObject result)
        {
            result = null;
            try
            {
                result = JObject.Parse(text);
                return true;
            }
            catch (JsonException ex)
            {
                component.AddError(
                    $"The input {inputName} must hold JSON objects: {ex.Message}");
                return false;
            }
        }
    }

    // The outputs of the components that read rules: one entry per rule, or
    // nulls and an error message for the error items of the response.
    public class GraphicOverrideRuleOutputs
    {
        public List<GraphicOverrideRuleGuidWrapper> RuleGuids =
            new List<GraphicOverrideRuleGuidWrapper>();

        public List<string> Names = new List<string>();
        public List<string> RuleGroupNames = new List<string>();
        public List<string> CriterionXMLs = new List<string>();
        public List<string> Styles = new List<string>();
        public List<string> ErrorMessages = new List<string>();

        public void Add(
            JToken rule)
        {
            if (JsonOutputHelp.IsError(rule))
            {
                RuleGuids.Add(null);
                Names.Add(null);
                RuleGroupNames.Add(null);
                CriterionXMLs.Add(null);
                Styles.Add(null);
                ErrorMessages.Add(JsonOutputHelp.ErrorMessage(rule));
                return;
            }

            RuleGuids.Add(GraphicOverrideHelp.RuleIdOf(rule?["graphicOverrideRuleId"]));
            Names.Add(rule?["name"]?.ToString());
            RuleGroupNames.Add(rule?["ruleGroupName"]?.ToString());
            CriterionXMLs.Add(rule?["criterionXML"]?.ToString());
            Styles.Add(rule?["style"]?.ToString(Formatting.None));
            ErrorMessages.Add("");
        }

        // Sets the outputs from firstIndex on, in the order of the fields above;
        // the error messages only when withErrors.
        public void SetOutputs(
            IGH_DataAccess da,
            int firstIndex,
            bool withErrors)
        {
            da.SetDataList(firstIndex, RuleGuids);
            da.SetDataList(firstIndex + 1, Names);
            da.SetDataList(firstIndex + 2, RuleGroupNames);
            da.SetDataList(firstIndex + 3, CriterionXMLs);
            da.SetDataList(firstIndex + 4, Styles);
            if (withErrors)
            {
                da.SetDataList(firstIndex + 5, ErrorMessages);
            }
        }
    }
}
