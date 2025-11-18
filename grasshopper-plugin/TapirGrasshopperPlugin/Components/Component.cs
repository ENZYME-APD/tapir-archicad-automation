using Grasshopper.GUI;
using Grasshopper.GUI.Canvas;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Attributes;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
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

            Rectangle componentCapsule = GH_Convert.ToRectangle(Bounds);
            componentCapsule.Width += _additionalWidth;
            componentCapsule.Height += _buttonBounds.Count * 30;

            for (int i = 0; i < _buttonBounds.Count; ++i)
            {
                Rectangle buttonCapsule = componentCapsule;
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
                    for (int i = 0; i < _buttonBounds.Count; ++i)
                    {
                        GH_Capsule buttonCapsule = GH_Capsule.CreateTextCapsule(
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
                for (int i = 0; i < _buttonBounds.Count; ++i)
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
                for (int i = 0; i < _buttonBounds.Count; ++i)
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

        public Component(
            string name,
            string nickname,
            string description,
            string subCategory)
            : base(
                name,
                nickname,
                description,
                "Tapir",
                subCategory)
        {
        }

        protected CommandResponse SendArchicadCommand(
            string commandName,
            JObject commandParameters)
        {
            ArchicadConnection connection = new(ConnectionSettings.Port);
            return connection.SendCommand(
                commandName,
                commandParameters);
        }

        protected CommandResponse SendArchicadAddOnCommand(
            string commandName,
            JObject commandParameters)
        {
            ArchicadConnection connection = new(ConnectionSettings.Port);

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
            CommandResponse cResponse = SendArchicadAddOnCommand(
                commandName,
                commandParameters);

            if (cResponse.Succeeded)
            {
                response = cResponse.Result;
                return true;
            }

            AddRuntimeMessage(
                GH_RuntimeMessageLevel.Error,
                cResponse.GetErrorMessage());
            response = null;
            return false;
        }

        protected bool GetResponse(
            string commandName,
            object commandParameters)
        {
            return GetArchicadAddonResponse(
                commandName,
                JObject.FromObject(commandParameters),
                out JObject result);
        }

        protected bool GetConvertedResponse<T>(
            string commandName,
            out T response)
            where T : class
        {
            if (GetArchicadAddonResponse(
                    commandName,
                    null,
                    out JObject result))
            {
                response = result.ToObject<T>();
                return true;
            }

            response = null;
            return false;
        }

        protected bool GetConvertedResponse<T>(
            string commandName,
            object commandParameters,
            out T response)
            where T : class
        {
            if (GetArchicadAddonResponse(
                    commandName,
                    JObject.FromObject(commandParameters),
                    out JObject result))
            {
                response = result.ToObject<T>();
                return true;
            }

            response = null;
            return false;
        }
    }

    public abstract class ArchicadAccessorComponent
        : Component, IButtonComponent
    {
        public abstract string CommandName { get; }

        public ArchicadAccessorComponent(
            string name,
            string nickname,
            string description,
            string subCategory)
            : base(
                name,
                nickname,
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

            foreach (IGH_Param input in Params.Input)
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
            if (!AutoRefresh && !ManualRefreshRequested)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Outdated, waiting for manual refresh");
                return;
            }

            Solve(DA);
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
            string nickname,
            string description,
            string subCategory)
            : base(
                name,
                nickname,
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

            foreach (IGH_Param input in Params.Input)
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
            string nickname,
            string description,
            string subCategory,
            string buttonText,
            int additionalWidth = 0)
            : base(
                name,
                nickname,
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