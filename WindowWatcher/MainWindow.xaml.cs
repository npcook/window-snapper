using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Automation;
using System.Windows.Interop;
using System.Runtime.InteropServices;
using System.Timers;
using System.Diagnostics;

using Thread = System.Threading.Thread;
using NotifyIcon = System.Windows.Forms.NotifyIcon;

namespace WindowWatcher
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		private Timer stopDraggingTimer = new Timer(100);
		private IntPtr ghostingWindowHandle = IntPtr.Zero;
		private AutomationElement ghostingElement = null;
		private Rect ghostingPreviousBounds = Rect.Empty;
		private ChangeType ghostingChangeType;
		private IntPtr hookHandle;
		private Process[] hookProcesses = new Process[2];

		private enum ChangeType
		{
			Move,
			Size,
		}

		private bool IsLeftMouseButtonDown()
		{
			return (Native.GetAsyncKeyState(Native.VirtualKeys.VK_LBUTTON) & 0x8000) != 0;
		}

		private void OnAutomationPropertyChanged(object sender, AutomationPropertyChangedEventArgs e)
		{
			if (!IsLeftMouseButtonDown())
				return;

			// We only care if there is a change in a bounding rectangle
			if (e.Property != AutomationElement.BoundingRectangleProperty)
				return;

			AutomationElement element;
			try
			{
				element = sender as AutomationElement;
			}
			catch
			{
				return;
			}

			if (element.Current.ProcessId == Process.GetCurrentProcess().Id || element.Current.ControlType != ControlType.Window)
				return;

			// Ignore windows that don't appear in the taskbar -- should exclude tooltips, popups, etc.
			var windowHandle = (IntPtr) element.Current.NativeWindowHandle;
			var styles = (Native.WindowStyles) Native.GetWindowLongPtr(windowHandle, Native.GetWindowLongPtrIndices.GWL_STYLE);
			if (styles.HasFlag(Native.WindowStyles.WS_CHILD))
				return;
			if (!(styles.HasFlag(Native.WindowStyles.WS_SYSMENU) && styles.HasFlag(Native.WindowStyles.WS_CAPTION)))
				return;

			// Ignore windows that are minimized or maximized
			var placement = Native.WINDOWPLACEMENT.Default;
			Native.GetWindowPlacement(windowHandle, ref placement);
			if (placement.showCmd == Native.WindowShowCommands.SW_SHOWMAXIMIZED || placement.showCmd == Native.WindowShowCommands.SW_SHOWMINIMIZED)
				return;

			Rect bounds = (e.NewValue as Rect?) ?? Rect.Empty;
			var _bounds = new Native.RECT(bounds);

			var monitorHandle = Native.MonitorFromRect(ref _bounds, Native.MonitorFromRectFlags.MONITOR_DEFAULTTONEAREST);

			var monitorInfo = Native.MONITORINFO.Default;
			bool success = Native.GetMonitorInfo(monitorHandle, ref monitorInfo);

			const int boundsOffset = 25;
			Rect monitor = new Rect()
			{
				X = monitorInfo.rcWork.left,
				Y = monitorInfo.rcWork.top,
				Width = monitorInfo.rcWork.right - monitorInfo.rcWork.left,
				Height = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top,
			};

			// Check to see if the window is within boundsOffset from the edge of a monitor
			var innerIntersection = monitor;
			var outerIntersection = monitor;
			innerIntersection.Inflate(-boundsOffset, -boundsOffset);
//			outerIntersection.Inflate(boundsOffset, boundsOffset);
			innerIntersection.Intersect(bounds);
			outerIntersection.Intersect(bounds);
			if (innerIntersection != bounds && outerIntersection == bounds)
			{
				Point newLocation = bounds.TopLeft;

				if (Math.Abs(monitor.Left - bounds.Left) < boundsOffset)
					newLocation.X = monitor.Left;
				else if (Math.Abs(monitor.Right - bounds.Right) < boundsOffset)
					newLocation.X = monitor.Right - bounds.Width;

				if (Math.Abs(monitor.Top - bounds.Top) < boundsOffset)
					newLocation.Y = monitor.Top;
				else if (Math.Abs(monitor.Bottom - bounds.Bottom) < boundsOffset)
					newLocation.Y = monitor.Bottom - bounds.Height;

				Dispatcher.BeginInvoke(new Action(() =>
				{
					Left = newLocation.X;
					Top = newLocation.Y;
					Width = bounds.Width;
					Height = bounds.Height;

					if (!IsVisible)
						Show();
				}));
				ghostingWindowHandle = windowHandle;
				ghostingElement = element;

				if (ghostingPreviousBounds != bounds)
				{
					if (ghostingPreviousBounds.Left != bounds.Left && ghostingPreviousBounds.Top != bounds.Top && ghostingPreviousBounds.Right != bounds.Right && ghostingPreviousBounds.Bottom != bounds.Bottom)
						ghostingChangeType = ChangeType.Move;
					else
						ghostingChangeType = ChangeType.Size;
					ghostingPreviousBounds = bounds;
				}
			}
			else
			{
				Dispatcher.BeginInvoke(new Action(() =>
				{
					if (IsVisible)
						Hide();
				}));
			}
			stopDraggingTimer.Start();
		}

		public MainWindow()
		{
			InitializeComponent();

			var trayIcon = new NotifyIcon();
			trayIcon.Icon = Properties.Resources.TrayIcon;
			trayIcon.Text = "Window Snapper";
			trayIcon.Visible = true;
			trayIcon.Click += (sender, args) => { Close(); };

//			IntPtr hookModule = Native.LoadLibrary("Hook.dll");
//			IntPtr hookProcAddress = Native.GetProcAddress(hookModule, "GetMsgProc");

			string[] processNames;
			if (Native.Is64BitOS())
				processNames = new string[2] {"SetHook32.exe", "SetHook64.exe"};
			else
				processNames = new string[1] {"SetHook32.exe"};
			for (int i = 0; i < hookProcesses.Length; ++i)
			{
				hookProcesses[i] = new Process();
				var hookProcess = hookProcesses[i];
				hookProcess.StartInfo = new ProcessStartInfo()
				{
					FileName = processNames[i],
					RedirectStandardInput = true,
					RedirectStandardOutput = true,
					UseShellExecute = false,
				};

				hookProcess.Start();
			}

//			hookHandle = Native.SetWindowsHookEx(Native.HookTypes.WH_GETMESSAGE, hookProcAddress, hookModule, 0);

//			stopDraggingTimer.Elapsed += OnStopDraggingTimerElapsed;
//			Automation.AddAutomationPropertyChangedEventHandler(AutomationElement.RootElement, TreeScope.Children, OnAutomationPropertyChanged, AutomationElement.BoundingRectangleProperty);
		}

		private void OnStopDraggingTimerElapsed(object sender, ElapsedEventArgs e)
		{
			return;
			if (!IsLeftMouseButtonDown())
			{
				stopDraggingTimer.Stop();

				Dispatcher.BeginInvoke(new Action(() =>
				{
					if (IsVisible)
					{
						Hide();

						if (ghostingElement != null)
						{
							object pattern;
							if (ghostingElement.TryGetCurrentPattern(TransformPattern.Pattern, out pattern))
							{
								Rect bounds = ghostingElement.Current.BoundingRectangle;
								if (ghostingChangeType == ChangeType.Size)
								{
									double width = Math.Max(bounds.Right, Left + Width - 1) - Math.Min(bounds.Left, Left);
									double height = Math.Max(bounds.Bottom, Top + Height - 1) - Math.Min(bounds.Top, Top);
									(pattern as TransformPattern).Resize(width, height);
								}

								(pattern as TransformPattern).Move(Left, Top);
							}
							ghostingElement = null;
						}
					}
				}));
			}
		}

		protected override void OnClosed(EventArgs e)
		{
			foreach (var hookProcess in hookProcesses)
			{
				hookProcess.StandardInput.BaseStream.Write(Encoding.ASCII.GetBytes("QUIT"), 0, 4);
				hookProcess.StandardInput.BaseStream.Flush();

				hookProcess.WaitForExit();
			}

			stopDraggingTimer.Stop();
			stopDraggingTimer.Dispose();
			Automation.RemoveAllEventHandlers();
			
			base.OnClosed(e);
		}

		protected override void OnSourceInitialized(EventArgs e)
		{
			base.OnSourceInitialized(e);

			var windowHandle = new WindowInteropHelper(this).Handle;
			var extendedStyle = (int) Native.GetWindowLongPtr(windowHandle, Native.GetWindowLongPtrIndices.GWL_EXSTYLE);
			Native.SetWindowLongPtr(windowHandle, Native.GetWindowLongPtrIndices.GWL_EXSTYLE, (IntPtr) (extendedStyle | 0x20));	// 0x20 = WS_EX_TRANSPARENT
		}
	}
}
