using Grasshopper.GUI;
using Grasshopper.GUI.Canvas;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Attributes;
using Newtonsoft.Json.Linq;
using System;
using System.Drawing;
using System.Windows.Forms;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components
{
    public interface IButtonComponent
    {
        void OnCapsuleButtonPressed ();

        string CapsuleButtonText { get; set; }
    }

    public class ButtonAttributes : GH_ComponentAttributes
    {
        #region Fields
        private bool _isPressed = false;
        private RectangleF _buttonBounds = new RectangleF ();
        #endregion Fields

        #region Constructor
        public ButtonAttributes (IGH_Component component) : base (component) { }
        #endregion Constructor

        #region Methods
        protected override void Layout ()
        {
            base.Layout ();

            Rectangle componentCapsule = GH_Convert.ToRectangle (this.Bounds);
            componentCapsule.Height += 30;

            Rectangle buttonCapsule = componentCapsule;
            buttonCapsule.Y = buttonCapsule.Bottom - 30;
            buttonCapsule.Height = 30;
            buttonCapsule.Inflate (-5, -5);

            this.Bounds = componentCapsule;
            this._buttonBounds = buttonCapsule;
        }

        protected override void Render (GH_Canvas canvas, Graphics graphics, GH_CanvasChannel channel)
        {
            base.Render (canvas, graphics, channel);

            if (channel == GH_CanvasChannel.Objects) {
                GH_Capsule buttonCapsule = GH_Capsule.CreateTextCapsule
                (
                    box: this._buttonBounds,
                    textbox: this._buttonBounds,
                    palette: this._isPressed ? GH_Palette.Grey : GH_Palette.Black,
                    text: this.Owner is IButtonComponent buttonComponent ? buttonComponent.CapsuleButtonText : "Refresh",
                    radius: 5,
                    highlight: 0);
                buttonCapsule.Render (graphics, this.Selected, this.Owner.Locked, false);
                buttonCapsule.Dispose ();
            }
        }

        public override GH_ObjectResponse RespondToMouseDown (GH_Canvas sender, GH_CanvasMouseEvent e)
        {
            if (e.Button == MouseButtons.Left) {
                if (this._buttonBounds.Contains (e.CanvasLocation)) {
                    this._isPressed = true;
                    sender.Refresh ();

                    return GH_ObjectResponse.Handled;
                }
            }
            return base.RespondToMouseDown (sender, e);
        }

        public override GH_ObjectResponse RespondToMouseUp (GH_Canvas sender, GH_CanvasMouseEvent e)
        {
            if (e.Button == System.Windows.Forms.MouseButtons.Left) {
                if (this._buttonBounds.Contains (e.CanvasLocation) && this._isPressed == true) {
                    if (this.Owner is IButtonComponent buttonComponent) {
                        buttonComponent.OnCapsuleButtonPressed ();
                        this._isPressed = false;
                        sender.Refresh ();
                    }
                    return GH_ObjectResponse.Release;
                }
            }
            return base.RespondToMouseUp (sender, e);
        }
        #endregion Methods
    }

    abstract public class Component : GH_Component
    {
        public Component (string name, string nickname, string description, string subCategory) :
            base (name, nickname, description, "Tapir", subCategory)
        {
        }

        protected CommandResponse SendArchicadCommand (string commandName, JObject commandParameters)
        {
            ArchicadConnection connection = new ArchicadConnection (ConnectionSettings.Port);
            return connection.SendCommand (commandName, commandParameters);
        }

        protected CommandResponse SendArchicadAddOnCommand (string commandNamespace, string commandName, JObject commandParameters)
        {
            ArchicadConnection connection = new ArchicadConnection (ConnectionSettings.Port);
            return connection.SendAddOnCommand (commandNamespace, commandName, commandParameters);
        }
    }

    abstract public class ArchicadAccessorComponent : Component, IButtonComponent
    {
        protected static bool AutoRefresh = true;
        protected static bool ManualRefreshWasExecuted = false;

        public ArchicadAccessorComponent (string name, string nickname, string description, string subCategory) :
            base (name, nickname, description, subCategory)
        {
            CapsuleButtonText = "Refresh";
        }

        protected override void AppendAdditionalComponentMenuItems (ToolStripDropDown menu)
        {
            base.AppendAdditionalComponentMenuItems (menu);
            Menu_AppendItem (menu, "Refresh from Archicad", OnRefreshButtonClicked);
        }

        public override void CreateAttributes ()
        {
            this.m_attributes = new ButtonAttributes (this);
        }

        public virtual void OnCapsuleButtonPressed ()
        {
            ManualRefresh ();
        }

        private void OnRefreshButtonClicked (object sender, EventArgs e)
        {
            ManualRefresh ();
        }

        public void ManualRefresh ()
        {
            ManualRefreshWasExecuted = true;
            try {
                ExpireSolution (true);
            } finally {
                ManualRefreshWasExecuted = false;
            }
        }

        protected override void ExpireDownStreamObjects ()
        {
            base.ExpireDownStreamObjects ();

            foreach (var input in Params.Input) {
                if (input is ArchicadAccessorValueList valueList) {
                    valueList.RefreshItems ();
                }
            }
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            if (!AutoRefresh && !ManualRefreshWasExecuted) {
                return;
            }

            Solve (DA);
        }

        protected abstract void Solve (IGH_DataAccess DA);

        public string CapsuleButtonText { get; set; }
    }
}
