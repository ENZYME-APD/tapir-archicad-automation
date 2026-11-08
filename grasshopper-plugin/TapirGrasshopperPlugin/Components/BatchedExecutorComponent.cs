using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.Linq;
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
    // the iteration it belongs to, so the outputs have the same shape as with
    // one request per branch.
    //
    // A component with a DryRun input (DryRunInputIndex) sends the iterations
    // with DryRun on in a second request with dryRun set, which changes
    // nothing; its refusals are warnings, and the OldValues and NewValues
    // outputs (OldValuesOutputIndex, NewValuesOutputIndex) get the wouldChange
    // values of each item.
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
            public bool DryRun;
        }

        private readonly List<Iteration> _iterations = new List<Iteration>();

        // The output the per-item error messages are written to (added with
        // OutErrorMessages), or -1 for a component without that output, which
        // reports the failed items in ReportBatchErrors instead.
        protected virtual int ErrorMessagesOutputIndex => 0;

        // The optional DryRun boolean input, or -1 when there is none.
        protected virtual int DryRunInputIndex => -1;

        // The outputs of the oldValue and newValue of each item's wouldChange
        // (null without DryRun), or -1 when there are none.
        protected virtual int OldValuesOutputIndex => -1;

        protected virtual int NewValuesOutputIndex => -1;

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

        // The per-item messages of the response written to the error messages
        // output, one per item sent.
        protected virtual List<string> GetBatchErrorMessages(
            JObject response,
            bool dryRun)
        {
            return ExecutionResultsResponse.ErrorMessages(response);
        }

        // Called with the items sent and the error message of each (empty
        // when it succeeded), for components that report failed items as
        // runtime messages rather than through an output.
        protected virtual void ReportBatchErrors(
            List<T> sentItems,
            List<string> messages)
        {
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
                    Path = ErrorMessagesOutputIndex >= 0
                        ? da.ParameterTargetPath(ErrorMessagesOutputIndex)
                        : null,
                    Items = items ?? new List<T>(),
                    DryRun = DryRunInputIndex >= 0 &&
                             da.GetOptional(
                                 DryRunInputIndex,
                                 false)
                });
        }

        protected override void AfterSolveInstance()
        {
            try
            {
                // A real run and a dry run cannot share a request, so the
                // iterations with DryRun on go in a second one.
                SendBatch(
                    _iterations.Where(x => !x.DryRun).ToList(),
                    false);
                SendBatch(
                    _iterations.Where(x => x.DryRun).ToList(),
                    true);
            }
            finally
            {
                _iterations.Clear();
                base.AfterSolveInstance();
            }
        }

        private void SendBatch(
            List<Iteration> iterations,
            bool dryRun)
        {
            if (iterations.Count == 0)
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
            foreach (var iteration in iterations)
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

            var batchParameters = CreateBatchParameters(sentItems);
            var parameters = batchParameters == null
                ? new JObject()
                : JObject.FromObject(batchParameters);

            // Only sent when it differs from the command's default.
            if (dryRun)
            {
                parameters["dryRun"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var messages = GetBatchErrorMessages(
                response,
                dryRun);
            if (messages.Count != sentItems.Count)
            {
                this.AddError(
                    $"The {CommandName} command answered {messages.Count} results " +
                    $"for {sentItems.Count} items.");
                return;
            }

            // A dry run changes nothing, so its refusals are reported, not
            // failed.
            if (dryRun)
            {
                AddDryRunWarnings(
                    ExecutionResultsResponse.ErrorMessages(response));
            }

            ReportBatchErrors(
                sentItems,
                messages);

            if (ErrorMessagesOutputIndex >= 0)
            {
                WriteSlices(
                    iterations,
                    resultIndexOfItem,
                    ErrorMessagesOutputIndex,
                    messages);
            }

            if (OldValuesOutputIndex >= 0)
            {
                WriteSlices(
                    iterations,
                    resultIndexOfItem,
                    OldValuesOutputIndex,
                    ExecutionResultsResponse.WouldChangeValues(
                        response,
                        "oldValue"));
            }

            if (NewValuesOutputIndex >= 0)
            {
                WriteSlices(
                    iterations,
                    resultIndexOfItem,
                    NewValuesOutputIndex,
                    ExecutionResultsResponse.WouldChangeValues(
                        response,
                        "newValue"));
            }
        }

        // Writes the results of each iteration's items to the iteration's
        // path of the output.
        private void WriteSlices(
            List<Iteration> iterations,
            List<int> resultIndexOfItem,
            int outputIndex,
            List<string> results)
        {
            var output = Params.Output[outputIndex];
            var itemIndex = 0;
            foreach (var iteration in iterations)
            {
                var branchResults = new List<string>();
                for (var i = 0; i < iteration.Items.Count; i++)
                {
                    var resultIndex = resultIndexOfItem[itemIndex];
                    branchResults.Add(
                        resultIndex < results.Count
                            ? results[resultIndex]
                            : null);
                    itemIndex++;
                }

                output.AddVolatileDataList(
                    iteration.Path,
                    branchResults);
            }
        }
    }
}
