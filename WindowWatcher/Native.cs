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
		// From: http://code.msdn.microsoft.com/windowsdesktop/CSCheckOSBitness-e18b1fe3#content

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
