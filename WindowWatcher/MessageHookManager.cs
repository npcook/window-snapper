using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.Windows;

using NotifyIcon = System.Windows.Forms.NotifyIcon;

namespace WindowWatcher
{
	class MessageHookManager : IDisposable
	{
		private Window mainWindow;
		private Process[] hookProcesses;
		private NotifyIcon trayIcon;
		private bool isDisposed = false;

		public MessageHookManager(Window mainWindow)
		{
			this.mainWindow = mainWindow;

			trayIcon = new NotifyIcon();
			trayIcon.Icon = Properties.Resources.TrayIcon;
			trayIcon.Text = "Window Snapper";
			trayIcon.Visible = true;
			trayIcon.Click += (sender, args) => { mainWindow.Close(); };

			string[] processNames;
			if (Native.Is64BitOS())
			{
				processNames = new string[2] { "SetHook32.exe", "SetHook64.exe" };
				hookProcesses = new Process[2];
			}
			else
			{
				processNames = new string[1] { "SetHook32.exe" };
				hookProcesses = new Process[1];
			}

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
		}

		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (isDisposed)
				return;
			isDisposed = true;

			if (disposing)
			{
				trayIcon.Dispose();

				foreach (var hookProcess in hookProcesses)
				{
					hookProcess.StandardInput.BaseStream.Write(Encoding.ASCII.GetBytes("QUIT"), 0, 4);
					hookProcess.StandardInput.BaseStream.Flush();

					hookProcess.WaitForExit();

					hookProcess.Dispose();
				}

				hookProcesses = null;
			}
		}
	}
}
