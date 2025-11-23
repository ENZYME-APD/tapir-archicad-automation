using Grasshopper.GUI;
using Grasshopper.GUI.Canvas;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Attributes;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components
{
    public interface IButtonComponent
    {
        void OnCapsuleButtonPressed(
            int buttonIndex);

        List<string> CapsuleButtonTexts { get; set; }
    }

    public class ButtonAttributes : GH_ComponentAttributes
    {
        #region Fields

        private List<bool> _isPressed;
        private List<RectangleF> _buttonBounds;
        private int _additionalWidth;

        #endregion Fields

        #region Constructor

        public ButtonAttributes(
            IGH_Component component,
            int buttonCount = 1,
            int additionalWidth = 0)
            : base(component)
        {
            _isPressed = new List<bool>(new bool[buttonCount]);
            _buttonBounds = new List<RectangleF>(new RectangleF[buttonCount]);
            _additionalWidth = additionalWidth;
        }

        #endregion Constructor

        #region Methods

        protected override void Layout()
        {
            base.Layout();

            var componentCapsule = GH_Convert.ToRectangle(Bounds);
            componentCapsule.Width += _additionalWidth;
            componentCapsule.Height += _buttonBounds.Count * 30;

            for (var i = 0; i < _buttonBounds.Count; ++i)
            {
                var buttonCapsule = componentCapsule;
                buttonCapsule.Y = buttonCapsule.Bottom - ((i + 1) * 30);
                buttonCapsule.Height = 30;
                buttonCapsule.Inflate(
                    -5,
                    -5);
                _buttonBounds[i] = buttonCapsule;
            }

            Bounds = componentCapsule;
        }

        protected override void Render(
            GH_Canvas canvas,
            Graphics graphics,
            GH_CanvasChannel channel)
        {
            base.Render(
                canvas,
                graphics,
                channel);

            if (channel == GH_CanvasChannel.Objects)
            {
                if (Owner is IButtonComponent buttonComponent)
                {
                    for (var i = 0; i < _buttonBounds.Count; ++i)
                    {
                        var buttonCapsule = GH_Capsule.CreateTextCapsule(
                            _buttonBounds[i],
                            _buttonBounds[i],
                            _isPressed[i] ? GH_Palette.Grey : GH_Palette.Black,
                            buttonComponent.CapsuleButtonTexts[i],
                            5,
                            0);
                        buttonCapsule.Render(
                            graphics,
                            Selected,
                            Owner.Locked,
                            false);
                        buttonCapsule.Dispose();
                    }
                }
            }
        }

        public override GH_ObjectResponse RespondToMouseDown(
            GH_Canvas sender,
            GH_CanvasMouseEvent e)
        {
            if (e.Button == MouseButtons.Left)
            {
                for (var i = 0; i < _buttonBounds.Count; ++i)
                {
                    if (_buttonBounds[i].Contains(e.CanvasLocation))
                    {
                        _isPressed[i] = true;
                        sender.Refresh();

                        return GH_ObjectResponse.Handled;
                    }
                }
            }

            return base.RespondToMouseDown(
                sender,
                e);
        }

        public override GH_ObjectResponse RespondToMouseUp(
            GH_Canvas sender,
            GH_CanvasMouseEvent e)
        {
            if (e.Button == MouseButtons.Left)
            {
                for (var i = 0; i < _buttonBounds.Count; ++i)
                {
                    if (_buttonBounds[i].Contains(e.CanvasLocation) &&
                        _isPressed[i] == true)
                    {
                        if (Owner is IButtonComponent buttonComponent)
                        {
                            buttonComponent.OnCapsuleButtonPressed(i);
                            _isPressed[i] = false;
                            sender.Refresh();
                        }

                        return GH_ObjectResponse.Release;
                    }
                }
            }

            return base.RespondToMouseUp(
                sender,
                e);
        }

        #endregion Methods
    }

    public abstract class Component : GH_Component
    {
        protected static bool AutoRefresh = true;
        public bool ManualRefreshRequested = false;
        protected static bool AutoExecute = false;
        public bool ManualExecuteRequested = false;

        private static string CommandNameSpace => "TapirCommand";

        protected GH_InputParamManager inManager;
        protected GH_OutputParamManager outManager;

        protected override void RegisterInputParams(
            GH_InputParamManager pm)
        {
            inManager = pm;
            AddInputs();
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pm)
        {
            outManager = pm;
            AddOutputs();
        }

        protected virtual void AddInputs() { }
        protected virtual void AddOutputs() { }

        public Component(
            string name,
            string description,
            string subCategory)
            : base(
                name,
                name,
                description,
                "Tapir",
                subCategory)
        {
        }

        public void InGeneric(
            string name,
            string description)
        {
            inManager.AddGenericParameter(
                name,
                name,
                description.WithType("genericItem"),
                GH_ParamAccess.item);
        }

        public void OutGeneric(
            string name,
            string description)
        {
            outManager.AddGenericParameter(
                name,
                name,
                description.WithType("genericItem"),
                GH_ParamAccess.item);
        }

        public void OutGenerics(
            string name,
            string description)
        {
            outManager.AddGenericParameter(
                name,
                name,
                description.WithType("genericList"),
                GH_ParamAccess.list);
        }

        public void InGenerics(
            string name,
            string description)
        {
            inManager.AddGenericParameter(
                name,
                name,
                description.WithType("genericList"),
                GH_ParamAccess.list);
        }

        public void InGenericTree(
            string name,
            string description)
        {
            inManager.AddGenericParameter(
                name,
                name,
                description.WithType("genericTree"),
                GH_ParamAccess.tree);
        }

        public void OutGenericTree(
            string name,
            string description)
        {
            outManager.AddGenericParameter(
                name,
                name,
                description.WithType("genericTree"),
                GH_ParamAccess.tree);
        }

        public void InVectors(
            string name,
            string description)
        {
            inManager.AddVectorParameter(
                name,
                name,
                description.WithType("vectorList"),
                GH_ParamAccess.list);
        }

        public void OutVectors(
            string name,
            string description)
        {
            outManager.AddVectorParameter(
                name,
                name,
                description.WithType("vectorList"),
                GH_ParamAccess.list);
        }

        public void InPoints(
            string name,
            string description)
        {
            inManager.AddPointParameter(
                name,
                name,
                description.WithType("pointList"),
                GH_ParamAccess.list);
        }

        public void OutPoints(
            string name,
            string description)
        {
            outManager.AddPointParameter(
                name,
                name,
                description.WithType("pointList"),
                GH_ParamAccess.list);
        }

        public void InNumber(
            string name,
            string description,
            double defaultValue)
        {
            inManager.AddNumberParameter(
                name,
                name,
                description.WithType("numberItem"),
                GH_ParamAccess.item,
                defaultValue);
        }

        public void OutNumbers(
            string name,
            string description)
        {
            outManager.AddNumberParameter(
                name,
                name,
                description.WithType("numberTree"),
                GH_ParamAccess.tree);
        }

        public void InNumbers(
            string name,
            string description)
        {
            inManager.AddNumberParameter(
                name,
                name,
                description.WithType("numberList"),
                GH_ParamAccess.list);
        }

        public void InBoolean(
            string name,
            string description,
            bool defaultValue = false)
        {
            inManager.AddBooleanParameter(
                name,
                name,
                description.WithType("booleanItem"),
                GH_ParamAccess.item,
                defaultValue);
        }

        public void InBooleans(
            string name,
            string description)
        {
            inManager.AddBooleanParameter(
                name,
                name,
                description.WithType("booleanList"),
                GH_ParamAccess.list);
        }

        public void InBooleanTree(
            string name,
            string description)
        {
            inManager.AddBooleanParameter(
                name,
                name,
                description.WithType("booleanTree"),
                GH_ParamAccess.tree);
        }

        public void OutBoolean(
            string name,
            string description)
        {
            outManager.AddBooleanParameter(
                name,
                name,
                description.WithType("booleanItem"),
                GH_ParamAccess.item);
        }

        public void OutBooleans(
            string name,
            string description)
        {
            outManager.AddBooleanParameter(
                name,
                name,
                description.WithType("booleanList"),
                GH_ParamAccess.list);
        }

        public void OutBooleanTree(
            string name,
            string description)
        {
            outManager.AddBooleanParameter(
                name,
                name,
                description.WithType("booleanTree"),
                GH_ParamAccess.tree);
        }

        public void InInteger(
            string name,
            string description,
            int defaultValue = 0)
        {
            inManager.AddIntegerParameter(
                name,
                name,
                description.WithType("integerItem"),
                GH_ParamAccess.item,
                defaultValue);
        }

        public void InIntegers(
            string name,
            string description)
        {
            inManager.AddIntegerParameter(
                name,
                name,
                description.WithType("integerList"),
                GH_ParamAccess.list);
        }

        public void InIntegerTree(
            string name,
            string description)
        {
            inManager.AddIntegerParameter(
                name,
                name,
                description.WithType("integerTree"),
                GH_ParamAccess.tree);
        }

        public void OutInteger(
            string name,
            string description)
        {
            outManager.AddIntegerParameter(
                name,
                name,
                description.WithType("integerItem"),
                GH_ParamAccess.item);
        }

        public void OutIntegers(
            string name,
            string description)
        {
            outManager.AddIntegerParameter(
                name,
                name,
                description.WithType("integerList"),
                GH_ParamAccess.list);
        }

        public void OutIntegerTree(
            string name,
            string description)
        {
            outManager.AddIntegerParameter(
                name,
                name,
                description.WithType("integerTree"),
                GH_ParamAccess.tree);
        }

        public void InText(
            string name,
            string description)
        {
            inManager.AddTextParameter(
                name,
                name,
                description.WithType("textItem"),
                GH_ParamAccess.item);
        }

        public void OutText(
            string name,
            string description)
        {
            outManager.AddTextParameter(
                name,
                name,
                description.WithType("textItem"),
                GH_ParamAccess.item);
        }

        public void OutTexts(
            string name,
            string description)
        {
            outManager.AddTextParameter(
                name,
                name,
                description.WithType("textList"),
                GH_ParamAccess.list);
        }

        public void OutTextTree(
            string name,
            string description)
        {
            outManager.AddTextParameter(
                name,
                name,
                description.WithType("textTree"),
                GH_ParamAccess.tree);
        }

        public void InColor(
            string name,
            string description)
        {
            inManager.AddColourParameter(
                name,
                name,
                description.WithType("colorItem"),
                GH_ParamAccess.item);
        }

        public void InColor(
            string name,
            string description,
            Color defaultValue)
        {
            inManager.AddColourParameter(
                name,
                name,
                description.WithType("colorItem"),
                GH_ParamAccess.item,
                defaultValue);
        }

        public void InColors(
            string name,
            string description,
            Color defaultValue)
        {
            inManager.AddColourParameter(
                name,
                name,
                description.WithType("colorList"),
                GH_ParamAccess.list,
                defaultValue);
        }

        public void OutColorTree(
            string name,
            string description)
        {
            outManager.AddColourParameter(
                name,
                name,
                description.WithType("colorTree"),
                GH_ParamAccess.tree);
        }

        public void InTexts(
            string name,
            string description)
        {
            inManager.AddTextParameter(
                name,
                name,
                description.WithType("textList"),
                GH_ParamAccess.list);
        }

        public void OutBoxes(
            string name,
            string description)
        {
            outManager.AddBoxParameter(
                name,
                name,
                description.WithType("boxList"),
                GH_ParamAccess.list);
        }

        public void OutCurves(
            string name,
            string description)
        {
            outManager.AddCurveParameter(
                name,
                name,
                description.WithType("curveList"),
                GH_ParamAccess.list);
        }

        protected CommandResponse SendArchicadCommand(
            string commandName,
            JObject commandParameters)
        {
            var connection = new ArchicadConnection(ConnectionSettings.Port);
            return connection.SendCommand(
                commandName,
                commandParameters);
        }

        protected CommandResponse SendArchicadAddOnCommand(
            string commandName,
            JObject commandParameters)
        {
            var connection = new ArchicadConnection(ConnectionSettings.Port);

            return connection.SendAddOnCommand(
                CommandNameSpace,
                commandName,
                commandParameters);
        }

        protected bool GetArchicadAddonResponse(
            string commandName,
            JObject commandParameters,
            out JObject response)
        {
            var cadResponse = SendArchicadAddOnCommand(
                commandName,
                commandParameters);

            if (cadResponse.Succeeded)
            {
                response = cadResponse.Result;
                return true;
            }

            AddRuntimeMessage(
                GH_RuntimeMessageLevel.Error,
                cadResponse.GetErrorMessage());

            response = null;
            return false;
        }

        protected bool SetArchiCadValues(
            string commandName,
            object commandParameters)
        {
            return GetArchicadAddonResponse(
                commandName,
                JObject.FromObject(commandParameters),
                out var result);
        }

        protected bool TryGetConvertedResponse<T>(
            string commandName,
            out T response)
            where T : class
        {
            return TryGetConvertedResponse(
                commandName,
                null,
                out response);
        }

        protected bool TryGetConvertedResponse<T>(
            string commandName,
            object commandParametersObject,
            out T response)
            where T : class
        {
            response = null;

            var commandParameters = commandParametersObject == null
                ? new JObject()
                : JObject.FromObject(commandParametersObject);

            if (!GetArchicadAddonResponse(
                    commandName,
                    commandParameters,
                    out var result))
            {
                return false;
            }

            try
            {
                response = result.ToObject<T>();
                return true;
            }
            catch (Exception ex)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    $"Failed to convert response to {typeof(T).Name}: {ex.Message}");
            }

            return false;
        }
    }

    public abstract class ArchicadAccessorComponent
        : Component, IButtonComponent
    {
        public abstract string CommandName { get; }

        public ArchicadAccessorComponent(
            string name,
            string description,
            string subCategory)
            : base(
                name,
                description,
                subCategory)
        {
            CapsuleButtonTexts = new List<string> { "Refresh" };
        }

        protected override void AppendAdditionalComponentMenuItems(
            ToolStripDropDown menu)
        {
            base.AppendAdditionalComponentMenuItems(menu);
            Menu_AppendItem(
                menu,
                "Refresh from Archicad",
                OnRefreshButtonClicked);
        }

        public override void CreateAttributes()
        {
            m_attributes = new ButtonAttributes(this);
        }

        public virtual void OnCapsuleButtonPressed(
            int buttonIndex)
        {
            ManualRefresh();
        }

        private void OnRefreshButtonClicked(
            object sender,
            EventArgs e)
        {
            ManualRefresh();
        }

        public void ManualRefresh()
        {
            ManualRefreshRequested = true;
            try
            {
                ExpireSolution(true);
            }
            finally
            {
                ManualRefreshRequested = false;
            }
        }

        protected override void ExpireDownStreamObjects()
        {
            base.ExpireDownStreamObjects();

            foreach (var input in Params.Input)
            {
                if (input is ArchicadAccessorValueList valueList)
                {
                    valueList.RefreshItems();
                }
            }
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            if (!AutoRefresh && !ManualRefreshRequested)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Outdated, waiting for manual refresh");
                return;
            }

            Solve(da);
        }

        protected abstract void Solve(
            IGH_DataAccess da);

        public List<string> CapsuleButtonTexts { get; set; }
    }

    public abstract class ArchicadExecutorComponent
        : Component, IButtonComponent
    {
        public abstract string CommandName { get; }

        public ArchicadExecutorComponent(
            string name,
            string description,
            string subCategory)
            : base(
                name,
                description,
                subCategory)
        {
            CapsuleButtonTexts = new List<string> { "Execute" };
        }

        protected override void AppendAdditionalComponentMenuItems(
            ToolStripDropDown menu)
        {
            base.AppendAdditionalComponentMenuItems(menu);
            Menu_AppendItem(
                menu,
                "Execute in Archicad",
                OnExecuteButtonClicked);
        }

        public override void CreateAttributes()
        {
            m_attributes = new ButtonAttributes(this);
        }

        public virtual void OnCapsuleButtonPressed(
            int buttonIndex)
        {
            ManualExecute();
        }

        private void OnExecuteButtonClicked(
            object sender,
            EventArgs e)
        {
            ManualExecute();
        }

        public void ManualExecute()
        {
            ManualExecuteRequested = true;
            try
            {
                ExpireSolution(true);
            }
            finally
            {
                ManualExecuteRequested = false;
            }
        }

        protected override void ExpireDownStreamObjects()
        {
            base.ExpireDownStreamObjects();

            foreach (var input in Params.Input)
            {
                if (input is ArchicadAccessorValueList valueList)
                {
                    valueList.RefreshItems();
                }
            }
        }

        protected override void SolveInstance(
            IGH_DataAccess DA)
        {
            if (!AutoExecute && !ManualExecuteRequested)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Waiting for manual execution");
                return;
            }

            Solve(DA);
        }

        protected abstract void Solve(
            IGH_DataAccess da);

        public List<string> CapsuleButtonTexts { get; set; }
    }

    public abstract class ButtonComponent : Component, IButtonComponent
    {
        private int _additionalWidth;

        public ButtonComponent(
            string name,
            string description,
            string subCategory,
            string buttonText,
            int additionalWidth = 0)
            : base(
                name,
                description,
                subCategory)
        {
            CapsuleButtonTexts = new List<string> { buttonText };
            _additionalWidth = additionalWidth;
            CreateAttributes();
        }

        public override void CreateAttributes()
        {
            m_attributes = new ButtonAttributes(
                this,
                2,
                _additionalWidth);
        }

        public abstract void OnCapsuleButtonPressed(
            int buttonIndex);

        public List<string> CapsuleButtonTexts { get; set; }
    }
}