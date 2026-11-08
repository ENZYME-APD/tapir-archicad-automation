using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components
{
    // An executor component that sends ONE request per solution instead of
    // one per Grasshopper iteration (#203). With tree inputs Grasshopper runs
    // SolveInstance once per branch combination; every run that changes
    // Archicad is one undo step there, and Archicad keeps only 100 of them.
    //
    // The inputs keep their item/list access, so Grasshopper still matches the
    // branches (longest list) and saved definitions keep working. Each
    // iteration only collects its request items (TryGetBatchItems) and the
    // output path it would have written to. AfterSolveInstance sends one
    // request with the items of every iteration, splits the per-item results
    // of the response in the same order and writes each slice to the path of
    // the iteration it belongs to, so the ErrorMessages tree has the same
    // shape as with one request per branch.
    //
    // An iteration whose inputs are invalid reports its error and adds no
    // items, like before; the other iterations are still sent.
    public abstract class BatchedExecutorComponent<T>
        : ArchicadExecutorComponent
    {
        private class Iteration
        {
            public GH_Path Path;
            public List<T> Items;
        }

        private readonly List<Iteration> _iterations = new List<Iteration>();

        // The output the per-item error messages are written to (added with
        // OutErrorMessages).
        protected virtual int ErrorMessagesOutputIndex => 0;

        public BatchedExecutorComponent(
            string name,
            string description,
            string subCategory)
            : base(
                name,
                description,
                subCategory)
        {
        }

        // Reads the inputs of one iteration and turns them into the items of
        // the request, in the order the command answers them. Returns false
        // (after adding an error) when the iteration cannot be sent.
        protected abstract bool TryGetBatchItems(
            IGH_DataAccess da,
            out List<T> items);

        // Builds the parameters of the one request from the items of every
        // iteration.
        protected abstract object CreateBatchParameters(
            List<T> items);

        // The target an item writes to (for example element and property),
        // or null when items never overwrite each other. Of the items with
        // the same key only the last one is sent.
        protected virtual string GetBatchItemKey(
            T item)
        {
            return null;
        }

        // The per-item error messages of the response, one per item sent.
        protected virtual List<string> GetBatchErrorMessages(
            JObject response)
        {
            return ExecutionResultsResponse.ErrorMessages(response);
        }

        protected override void BeforeSolveInstance()
        {
            base.BeforeSolveInstance();
            _iterations.Clear();
        }

        protected sealed override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetBatchItems(
                    da,
                    out var items))
            {
                return;
            }

            _iterations.Add(
                new Iteration
                {
                    Path = da.ParameterTargetPath(ErrorMessagesOutputIndex),
                    Items = items ?? new List<T>()
                });
        }

        protected override void AfterSolveInstance()
        {
            try
            {
                SendBatch();
            }
            finally
            {
                _iterations.Clear();
                base.AfterSolveInstance();
            }
        }

        private void SendBatch()
        {
            if (_iterations.Count == 0)
            {
                return;
            }

            // With one request per branch a later branch overwrote what an
            // earlier one set on the same target; in one request the command
            // may refuse or ignore the repeated target. So only the last item
            // of each key is sent, and every item of that key gets its result.
            var sentItems = new List<T>();
            var resultIndexOfItem = new List<int>();
            var sentIndexOfKey = new Dictionary<string, int>();
            foreach (var iteration in _iterations)
            {
                foreach (var item in iteration.Items)
                {
                    var key = GetBatchItemKey(item);
                    if (key != null &&
                        sentIndexOfKey.TryGetValue(
                            key,
                            out var sentIndex))
                    {
                        sentItems[sentIndex] = item;
                        resultIndexOfItem.Add(sentIndex);
                        continue;
                    }

                    if (key != null)
                    {
                        sentIndexOfKey[key] = sentItems.Count;
                    }

                    resultIndexOfItem.Add(sentItems.Count);
                    sentItems.Add(item);
                }
            }

            if (sentItems.Count == 0)
            {
                return;
            }

            var parameters = CreateBatchParameters(sentItems);
            if (!TryGetCadResponse(
                    CommandName,
                    parameters == null
                        ? new JObject()
                        : JObject.FromObject(parameters),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var messages = GetBatchErrorMessages(response);
            if (messages.Count != sentItems.Count)
            {
                this.AddError(
                    $"The {CommandName} command answered {messages.Count} results " +
                    $"for {sentItems.Count} items.");
                return;
            }

            var output = Params.Output[ErrorMessagesOutputIndex];
            var itemIndex = 0;
            foreach (var iteration in _iterations)
            {
                var branchMessages = new List<string>();
                for (var i = 0; i < iteration.Items.Count; i++)
                {
                    branchMessages.Add(
                        messages[resultIndexOfItem[itemIndex]]);
                    itemIndex++;
                }

                output.AddVolatileDataList(
                    iteration.Path,
                    branchMessages);
            }
        }
    }
}
