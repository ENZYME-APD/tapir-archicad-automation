using TapirGrasshopperPlugin.ResponseTypes.Generic;

namespace TapirGrasshopperPlugin.Components.TeamworkComponents
{
    public abstract class AbsTeamworkBaseComponent : ArchicadAccessorComponent
    {
        protected AbsTeamworkBaseComponent(
            string name,
            string description)
            : base(
                name,
                description,
                GroupNames.Teamwork)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult),
                ExecutionResult.Doc);
        }
    }
}