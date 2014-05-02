using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace WindowWatcher
{
	internal static class Native
	{
		// From pinvoke.net

		public const uint WINEVENT_OUTOFCONTEXT = 0x0000; // Events are ASYNC
		public const uint WINEVENT_SKIPOWNTHREAD = 0x0001; // Don't call back for events on installer's thread
		public const uint WINEVENT_SKIPOWNPROCESS = 0x0002; // Don't call back for events on installer's process
		public const uint WINEVENT_INCONTEXT = 0x0004; // Events are SYNC, this causes your dll to be injected into every process
		public const uint EVENT_MIN = 0x00000001;
		public const uint EVENT_MAX = 0x7FFFFFFF;
		public const uint EVENT_SYSTEM_SOUND = 0x0001;
		public const uint EVENT_SYSTEM_ALERT = 0x0002;
		public const uint EVENT_SYSTEM_FOREGROUND = 0x0003;
		public const uint EVENT_SYSTEM_MENUSTART = 0x0004;
		public const uint EVENT_SYSTEM_MENUEND = 0x0005;
		public const uint EVENT_SYSTEM_MENUPOPUPSTART = 0x0006;
		public const uint EVENT_SYSTEM_MENUPOPUPEND = 0x0007;
		public const uint EVENT_SYSTEM_CAPTURESTART = 0x0008;
		public const uint EVENT_SYSTEM_CAPTUREEND = 0x0009;
		public const uint EVENT_SYSTEM_MOVESIZESTART = 0x000A;
		public const uint EVENT_SYSTEM_MOVESIZEEND = 0x000B;
		public const uint EVENT_SYSTEM_CONTEXTHELPSTART = 0x000C;
		public const uint EVENT_SYSTEM_CONTEXTHELPEND = 0x000D;
		public const uint EVENT_SYSTEM_DRAGDROPSTART = 0x000E;
		public const uint EVENT_SYSTEM_DRAGDROPEND = 0x000F;
		public const uint EVENT_SYSTEM_DIALOGSTART = 0x0010;
		public const uint EVENT_SYSTEM_DIALOGEND = 0x0011;
		public const uint EVENT_SYSTEM_SCROLLINGSTART = 0x0012;
		public const uint EVENT_SYSTEM_SCROLLINGEND = 0x0013;
		public const uint EVENT_SYSTEM_SWITCHSTART = 0x0014;
		public const uint EVENT_SYSTEM_SWITCHEND = 0x0015;
		public const uint EVENT_SYSTEM_MINIMIZESTART = 0x0016;
		public const uint EVENT_SYSTEM_MINIMIZEEND = 0x0017;
		public const uint EVENT_SYSTEM_DESKTOPSWITCH = 0x0020;
		public const uint EVENT_SYSTEM_END = 0x00FF;
		public const uint EVENT_OEM_DEFINED_START = 0x0101;
		public const uint EVENT_OEM_DEFINED_END = 0x01FF;
		public const uint EVENT_UIA_EVENTID_START = 0x4E00;
		public const uint EVENT_UIA_EVENTID_END = 0x4EFF;
		public const uint EVENT_UIA_PROPID_START = 0x7500;
		public const uint EVENT_UIA_PROPID_END = 0x75FF;
		public const uint EVENT_CONSOLE_CARET = 0x4001;
		public const uint EVENT_CONSOLE_UPDATE_REGION = 0x4002;
		public const uint EVENT_CONSOLE_UPDATE_SIMPLE = 0x4003;
		public const uint EVENT_CONSOLE_UPDATE_SCROLL = 0x4004;
		public const uint EVENT_CONSOLE_LAYOUT = 0x4005;
		public const uint EVENT_CONSOLE_START_APPLICATION = 0x4006;
		public const uint EVENT_CONSOLE_END_APPLICATION = 0x4007;
		public const uint EVENT_CONSOLE_END = 0x40FF;
		public const uint EVENT_OBJECT_CREATE = 0x8000;
		public const uint EVENT_OBJECT_DESTROY = 0x8001;
		public const uint EVENT_OBJECT_SHOW = 0x8002;
		public const uint EVENT_OBJECT_HIDE = 0x8003;
		public const uint EVENT_OBJECT_REORDER = 0x8004;
		public const uint EVENT_OBJECT_FOCUS = 0x8005;
		public const uint EVENT_OBJECT_SELECTION = 0x8006;
		public const uint EVENT_OBJECT_SELECTIONADD = 0x8007;
		public const uint EVENT_OBJECT_SELECTIONREMOVE = 0x8008;
		public const uint EVENT_OBJECT_SELECTIONWITHIN = 0x8009;
		public const uint EVENT_OBJECT_STATECHANGE = 0x800A;
		public const uint EVENT_OBJECT_LOCATIONCHANGE = 0x800B;
		public const uint EVENT_OBJECT_NAMECHANGE = 0x800C;
		public const uint EVENT_OBJECT_DESCRIPTIONCHANGE = 0x800D;
		public const uint EVENT_OBJECT_VALUECHANGE = 0x800E;
		public const uint EVENT_OBJECT_PARENTCHANGE = 0x800F;
		public const uint EVENT_OBJECT_HELPCHANGE = 0x8010;
		public const uint EVENT_OBJECT_DEFACTIONCHANGE = 0x8011;
		public const uint EVENT_OBJECT_ACCELERATORCHANGE = 0x8012;
		public const uint EVENT_OBJECT_INVOKED = 0x8013;
		public const uint EVENT_OBJECT_TEXTSELECTIONCHANGED = 0x8014;
		public const uint EVENT_OBJECT_CONTENTSCROLLED = 0x8015;
		public const uint EVENT_SYSTEM_ARRANGMENTPREVIEW = 0x8016;
		public const uint EVENT_OBJECT_END = 0x80FF;
		public const uint EVENT_AIA_START = 0xA000;
		public const uint EVENT_AIA_END = 0xAFFF;

		internal delegate void WinEventDelegate(IntPtr hwinEventHook, uint eventType, IntPtr hwnd, int idObject, int idChild, uint dwEventThread, uint dwmsEventTime);

		[DllImport("user32.dll")]
		internal static extern IntPtr SetWinEventHook(uint eventMin, uint eventMax, IntPtr hmodWinEvenProc, WinEventDelegate lpfnWinEventProc, uint idProcess, uint idThread, uint dwFlags);

		[DllImport("user32.dll")]
		internal static extern bool UnhookWinEvent(IntPtr hWinEventHook);

		[DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
		internal static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

		internal static class SetWindowPosHandles
		{
			public static readonly IntPtr HWND_BOTTOM = (IntPtr) 1;
			public static readonly IntPtr HWND_NOTOPMOST = (IntPtr) (-2);
			public static readonly IntPtr HWND_TOP = (IntPtr) 0;
			public static readonly IntPtr HWND_TOPMOST = (IntPtr) (-1);
		}

		[Flags()]
		internal enum SetWindowPosFlags
		{
			SWP_ASYNCWINDOWPOS = 0x4000,
			SWP_DEFERERASE = 0x2000,
			SWP_DRAWFRAME = 0x0020,
			SWP_FRAMECHANGED = 0x0020,
			SWP_HIDEWINDOW = 0x0080,
			SWP_NOACTIVATE = 0x0010,
			SWP_NOCOPYBITS = 0x0100,
			SWP_NOMOVE = 0x0002,
			SWP_NOOWNERZORDER = 0x0200,
			SWP_NOREDRAW = 0x0008,
			SWP_NOREPOSITION = 0x0200,
			SWP_NOSENDCHANGING = 0x0400,
			SWP_NOSIZE = 0x0001,
			SWP_NOZORDER = 0x0004,
			SWP_SHOWWINDOW = 0x0040,
		}

		[DllImport("user32.dll", SetLastError = true)]
		internal static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int x, int y, int cx, int cy, SetWindowPosFlags uFlags);

		[StructLayout(LayoutKind.Sequential)]
		internal struct RECT
		{
			public int left;
			public int top;
			public int right;
			public int bottom;

			public RECT(System.Windows.Rect rect)
			{
				left = (int) rect.Left;
				top = (int) rect.Top;
				right = (int) rect.Right;
				bottom = (int) rect.Bottom;
			}
		}

		[StructLayout(LayoutKind.Sequential)]
		internal struct POINT
		{
			public int x;
			public int y;

			public POINT(System.Windows.Point point)
			{
				x = (int) point.X;
				y = (int) point.Y;
			}
		}

		[StructLayout(LayoutKind.Sequential)]
		internal struct MONITORINFO
		{
			public int cbSize;
			public RECT rcMonitor;
			public RECT rcWork;
			public uint dwFlags;

			public static MONITORINFO Default
			{
				get
				{
					MONITORINFO result = new MONITORINFO();
					result.cbSize = Marshal.SizeOf(result);
					return result;
				}
			}
		}

		[DllImport("user32.dll")]
		internal static extern bool GetMonitorInfo(IntPtr hMonitor, ref MONITORINFO lpmi);

		internal enum MonitorFromRectFlags
		{
			MONITOR_DEFAULTTONEAREST = 2,
			MONITOR_DEFAULTTOPRIMARY = 1,
			MONITOR_DEFAULTTONULL = 0,
		}

		[DllImport("user32.dll")]
		internal static extern IntPtr MonitorFromRect([In] ref RECT lprc, MonitorFromRectFlags dwFlags);

		// From: pinvoke.net
		
		/// <summary>
		/// Window Styles.
		/// The following styles can be specified wherever a window style is required. After the control has been created, these styles cannot be modified, except as noted.
		/// </summary>
		[Flags()]
		internal enum WindowStyles : uint
		{
			/// <summary>The window has a thin-line border.</summary>
			WS_BORDER = 0x800000,

			/// <summary>The window has a title bar (includes the WS_BORDER style).</summary>
			WS_CAPTION = 0xc00000,

			/// <summary>The window is a child window. A window with this style cannot have a menu bar. This style cannot be used with the WS_POPUP style.</summary>
			WS_CHILD = 0x40000000,

			/// <summary>Excludes the area occupied by child windows when drawing occurs within the parent window. This style is used when creating the parent window.</summary>
			WS_CLIPCHILDREN = 0x2000000,

			/// <summary>
			/// Clips child windows relative to each other; that is, when a particular child window receives a WM_PAINT message, the WS_CLIPSIBLINGS style clips all other overlapping child windows out of the region of the child window to be updated.
			/// If WS_CLIPSIBLINGS is not specified and child windows overlap, it is possible, when drawing within the client area of a child window, to draw within the client area of a neighboring child window.
			/// </summary>
			WS_CLIPSIBLINGS = 0x4000000,

			/// <summary>The window is initially disabled. A disabled window cannot receive input from the user. To change this after a window has been created, use the EnableWindow function.</summary>
			WS_DISABLED = 0x8000000,

			/// <summary>The window has a border of a style typically used with dialog boxes. A window with this style cannot have a title bar.</summary>
			WS_DLGFRAME = 0x400000,

			/// <summary>
			/// The window is the first control of a group of controls. The group consists of this first control and all controls defined after it, up to the next control with the WS_GROUP style.
			/// The first control in each group usually has the WS_TABSTOP style so that the user can move from group to group. The user can subsequently change the keyboard focus from one control in the group to the next control in the group by using the direction keys.
			/// You can turn this style on and off to change dialog box navigation. To change this style after a window has been created, use the SetWindowLong function.
			/// </summary>
			WS_GROUP = 0x20000,

			/// <summary>The window has a horizontal scroll bar.</summary>
			WS_HSCROLL = 0x100000,

			/// <summary>The window is initially maximized.</summary>
			WS_MAXIMIZE = 0x1000000,

			/// <summary>The window has a maximize button. Cannot be combined with the WS_EX_CONTEXTHELP style. The WS_SYSMENU style must also be specified.</summary>
			WS_MAXIMIZEBOX = 0x10000,

			/// <summary>The window is initially minimized.</summary>
			WS_MINIMIZE = 0x20000000,

			/// <summary>The window has a minimize button. Cannot be combined with the WS_EX_CONTEXTHELP style. The WS_SYSMENU style must also be specified.</summary>
			WS_MINIMIZEBOX = 0x20000,

			/// <summary>The window is an overlapped window. An overlapped window has a title bar and a border.</summary>
			WS_OVERLAPPED = 0x0,

			/// <summary>The window is an overlapped window.</summary>
			WS_OVERLAPPEDWINDOW = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,

			/// <summary>The window is a pop-up window. This style cannot be used with the WS_CHILD style.</summary>
			WS_POPUP = 0x80000000u,

			/// <summary>The window is a pop-up window. The WS_CAPTION and WS_POPUPWINDOW styles must be combined to make the window menu visible.</summary>
			WS_POPUPWINDOW = WS_POPUP | WS_BORDER | WS_SYSMENU,

			/// <summary>The window has a sizing border.</summary>
			WS_SIZEFRAME = 0x40000,

			/// <summary>The window has a window menu on its title bar. The WS_CAPTION style must also be specified.</summary>
			WS_SYSMENU = 0x80000,

			/// <summary>
			/// The window is a control that can receive the keyboard focus when the user presses the TAB key.
			/// Pressing the TAB key changes the keyboard focus to the next control with the WS_TABSTOP style.  
			/// You can turn this style on and off to change dialog box navigation. To change this style after a window has been created, use the SetWindowLong function.
			/// For user-created windows and modeless dialogs to work with tab stops, alter the message loop to call the IsDialogMessage function.
			/// </summary>
			WS_TABSTOP = 0x10000,

			/// <summary>The window is initially visible. This style can be turned on and off by using the ShowWindow or SetWindowPos function.</summary>
			WS_VISIBLE = 0x10000000,

			/// <summary>The window has a vertical scroll bar.</summary>
			WS_VSCROLL = 0x200000
		}

		internal static class GetWindowLongPtrIndices
		{
			public static readonly int GWLP_WNDPROC = -4;
			public static readonly int GWLP_HINSTANCE = -6;
			public static readonly int GWLP_HWNDPARENT = -8;
			public static readonly int GWLP_ID = -12;
			public static readonly int GWL_STYLE = -16;
			public static readonly int GWL_EXSTYLE = -20;
			public static readonly int GWLP_USERDATA = -21;
		}

		[DllImport("user32.dll", EntryPoint = "GetWindowLong", SetLastError = true)]
		private static extern IntPtr GetWindowLongPtr32(IntPtr hWnd, int nIndex);

		[DllImport("user32.dll", EntryPoint = "GetWindowLongPtr", SetLastError = true)]
		private static extern IntPtr GetWindowLongPtr64(IntPtr hWnd, int nIndex);

		internal static IntPtr GetWindowLongPtr(IntPtr hWnd, int nIndex)
		{
			// If A pointer is 8 bytes, we are on 64 bit
			if (IntPtr.Size == 8)
				return GetWindowLongPtr64(hWnd, nIndex);
			else
				return GetWindowLongPtr32(hWnd, nIndex);
		}

		[DllImport("user32.dll", EntryPoint = "SetWindowLong", SetLastError = true)]
		private static extern IntPtr SetWindowLongPtr32(IntPtr hWnd, int nIndex, IntPtr dwNewLong);

		[DllImport("user32.dll", EntryPoint = "SetWindowLongPtr", SetLastError = true)]
		private static extern IntPtr SetWindowLongPtr64(IntPtr hWnd, int nIndex, IntPtr dwNewLong);

		internal static IntPtr SetWindowLongPtr(IntPtr hWnd, int nIndex, IntPtr dwNewLong)
		{
			// If A pointer is 8 bytes, we are on 64 bit
			if (IntPtr.Size == 8)
				return SetWindowLongPtr64(hWnd, nIndex, dwNewLong);
			else
				return SetWindowLongPtr32(hWnd, nIndex, dwNewLong);
		}

		internal enum WindowShowCommands : uint
		{
			SW_HIDE = 0,
			SW_MAXIMIZE = 3,
			SW_MINIMIZE = 6,
			SW_RESTORE = 9,
			SW_SHOW = 5,
			SW_SHOWMAXIMIZED = 3,
			SW_SHOWMINIMIZED = 2,
			SW_SHOWMINNOACTIVE = 7,
			SW_SHOWNA = 8,
			SW_SHOWNOACTIVE = 4,
			SW_SHOWNORMAL = 1,
		}

		[StructLayout(LayoutKind.Sequential)]
		internal struct WINDOWPLACEMENT
		{
			/// <summary>
			/// The length of the structure, in bytes. Before calling the GetWindowPlacement or SetWindowPlacement functions, set this member to sizeof(WINDOWPLACEMENT).
			/// <para>
			/// GetWindowPlacement and SetWindowPlacement fail if this member is not set correctly.
			/// </para>
			/// </summary>
			public int length;

			/// <summary>
			/// Specifies flags that control the position of the minimized window and the method by which the window is restored.
			/// </summary>
			public int flags;

			/// <summary>
			/// The current show state of the window.
			/// </summary>
			public WindowShowCommands showCmd;

			/// <summary>
			/// The coordinates of the window's upper-left corner when the window is minimized.
			/// </summary>
			public POINT ptMinPosition;

			/// <summary>
			/// The coordinates of the window's upper-left corner when the window is maximized.
			/// </summary>
			public POINT ptMaxPosition;

			/// <summary>
			/// The window's coordinates when the window is in the restored position.
			/// </summary>
			public RECT rcNormalPosition;

			/// <summary>
			/// Gets the default (empty) value.
			/// </summary>
			public static WINDOWPLACEMENT Default
			{
				get
				{
					WINDOWPLACEMENT result = new WINDOWPLACEMENT();
					result.length = Marshal.SizeOf(result);
					return result;
				}
			}
		}

		[DllImport("user32.dll", SetLastError = true)]
		internal static extern bool GetWindowPlacement(IntPtr hWnd, ref WINDOWPLACEMENT lpwndpl);

		internal enum VirtualKeys : int
		{
			VK_LBUTTON = 0x01,
			VK_RBUTTON = 0x02,
		}

		[DllImport("user32.dll", SetLastError = true)]
		internal static extern short GetAsyncKeyState(VirtualKeys nVirtKey);

		internal enum HookTypes : int
		{
			WH_GETMESSAGE = 3,
		}

		/// <summary>
		/// The function determines whether the current operating system is a 
		/// 64-bit operating system.
		/// </summary>
		/// <returns>
		/// The function returns true if the operating system is 64-bit; 
		/// otherwise, it returns false.
		/// </returns>
		public static bool Is64BitOS()
		{
			if (IntPtr.Size == 8)  // 64-bit programs run only on Win64
			{
				return true;
			}
			else  // 32-bit programs run on both 32-bit and 64-bit Windows
			{
				// Detect whether the current process is a 32-bit process 
				// running on a 64-bit system.
				bool flag;
				return ((DoesWin32MethodExist("kernel32.dll", "IsWow64Process") &&
					IsWow64Process(GetCurrentProcess(), out flag)) && flag);
			}
		}

		/// <summary>
		/// The function determins whether a method exists in the export 
		/// table of a certain module.
		/// </summary>
		/// <param name="moduleName">The name of the module</param>
		/// <param name="methodName">The name of the method</param>
		/// <returns>
		/// The function returns true if the method specified by methodName 
		/// exists in the export table of the module specified by moduleName.
		/// </returns>
		static bool DoesWin32MethodExist(string moduleName, string methodName)
		{
			IntPtr moduleHandle = GetModuleHandle(moduleName);
			if (moduleHandle == IntPtr.Zero)
			{
				return false;
			}
			return (GetProcAddress(moduleHandle, methodName) != IntPtr.Zero);
		}

		[DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
		static extern IntPtr GetCurrentProcess();

		[DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
		static extern IntPtr GetModuleHandle(string moduleName);

		[DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
		static extern IntPtr GetProcAddress(IntPtr hModule,
			[MarshalAs(UnmanagedType.LPStr)]string procName);

		[DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		static extern bool IsWow64Process(IntPtr hProcess, out bool wow64Process);
	}
}
