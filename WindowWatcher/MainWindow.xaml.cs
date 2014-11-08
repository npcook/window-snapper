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

namespace WindowSnapper
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		MessageHookManager hookManager;

		public MainWindow()
		{
			InitializeComponent();

			hookManager = new MessageHookManager(this);
		}

		protected override void OnClosed(EventArgs e)
		{
			if (hookManager != null)
			{
				hookManager.Dispose();
				hookManager = null;
			}
			
			base.OnClosed(e);
		}
	}
}
