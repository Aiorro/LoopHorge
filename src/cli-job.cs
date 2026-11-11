// Windows CLI ownership: the job kills descendants on normal exit or cancellation.
// Windows 10+ job-list attributes assign ownership atomically at process creation.
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;
public static class LoopForgeCliJob {
 [StructLayout(LayoutKind.Sequential)] struct IO { public ulong Read, Write, Other, ReadBytes, WriteBytes, OtherBytes; }
 [StructLayout(LayoutKind.Sequential)] struct Basic { public long ProcessTime, JobTime; public uint Flags; public UIntPtr MinWorking, MaxWorking; public uint ActiveProcesses; public UIntPtr Affinity; public uint Priority, Scheduling; }
 [StructLayout(LayoutKind.Sequential)] struct Limits { public Basic Basic; public IO IO; public UIntPtr ProcessMemory, JobMemory, PeakProcess, PeakJob; }
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] struct Startup { public uint Size; public string Reserved, Desktop, Title; public uint X,Y,XSize,YSize,XChars,YChars,Fill,Flags; public ushort Show,ReservedSize; public IntPtr ReservedData,Input,Output,Error; }
 [StructLayout(LayoutKind.Sequential)] struct StartupEx { public Startup Startup; public IntPtr Attributes; }
 [StructLayout(LayoutKind.Sequential)] struct ProcessInfo { public IntPtr Process,Thread; public uint Pid,Tid; }
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr CreateJobObject(IntPtr attributes,string name);
 [DllImport("kernel32.dll",SetLastError=true)] static extern bool SetInformationJobObject(IntPtr job,int type,ref Limits info,uint size);
 [DllImport("kernel32.dll",SetLastError=true)] static extern bool InitializeProcThreadAttributeList(IntPtr list,int count,uint flags,ref UIntPtr bytes);
 [DllImport("kernel32.dll",SetLastError=true)] static extern bool UpdateProcThreadAttribute(IntPtr list,uint flags,UIntPtr attribute,IntPtr value,UIntPtr size,IntPtr previous,IntPtr returned);
 [DllImport("kernel32.dll")] static extern void DeleteProcThreadAttributeList(IntPtr list);
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern bool CreateProcess(string app,StringBuilder command,IntPtr processAttrs,IntPtr threadAttrs,bool inherit,uint flags,IntPtr environment,string cwd,ref StartupEx startup,out ProcessInfo process);
 [DllImport("kernel32.dll",SetLastError=true)] static extern uint ResumeThread(IntPtr thread);
 [DllImport("kernel32.dll")] static extern uint WaitForSingleObject(IntPtr handle,uint timeout);
 [DllImport("kernel32.dll",SetLastError=true)] static extern bool GetExitCodeProcess(IntPtr process,out uint code);
 [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr handle);
 [DllImport("kernel32.dll")] static extern IntPtr GetStdHandle(int value);
 [DllImport("kernel32.dll",SetLastError=true)] static extern bool SetHandleInformation(IntPtr handle,uint mask,uint flags);
 static void Require(bool success) { if(!success)throw new Win32Exception(Marshal.GetLastWin32Error()); }
 public static int Run() {
  IntPtr job=CreateJobObject(IntPtr.Zero,null);if(job==IntPtr.Zero)throw new Win32Exception();
  ProcessInfo process=new ProcessInfo();IntPtr attributes=IntPtr.Zero,jobValue=IntPtr.Zero;bool initialized=false;
  try {
   Limits limits=new Limits();limits.Basic.Flags=0x2000; // JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
   Require(SetInformationJobObject(job,9,ref limits,(uint)Marshal.SizeOf(typeof(Limits))));
   UIntPtr bytes=UIntPtr.Zero;InitializeProcThreadAttributeList(IntPtr.Zero,1,0,ref bytes);
   attributes=Marshal.AllocHGlobal(checked((int)bytes.ToUInt64()));
   Require(InitializeProcThreadAttributeList(attributes,1,0,ref bytes));initialized=true;
   jobValue=Marshal.AllocHGlobal(IntPtr.Size);Marshal.WriteIntPtr(jobValue,job);
   Require(UpdateProcThreadAttribute(attributes,0,new UIntPtr(0x0002000d),jobValue,new UIntPtr((uint)IntPtr.Size),IntPtr.Zero,IntPtr.Zero));
   StartupEx startup=new StartupEx();startup.Attributes=attributes;startup.Startup.Size=(uint)Marshal.SizeOf(typeof(StartupEx));startup.Startup.Flags=0x100;
   startup.Startup.Input=GetStdHandle(-10);startup.Startup.Output=GetStdHandle(-11);startup.Startup.Error=GetStdHandle(-12);
   Require(SetHandleInformation(startup.Startup.Input,1,1));Require(SetHandleInformation(startup.Startup.Output,1,1));Require(SetHandleInformation(startup.Startup.Error,1,1));
   string cmd=System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),"cmd.exe");
   // LOOPFORGE_JOB_COMMAND contains only fixed adapter flags and a validated model ID.
   string command=Environment.GetEnvironmentVariable("LOOPFORGE_JOB_COMMAND");
   Require(CreateProcess(cmd,new StringBuilder("\""+cmd+"\" /d /s /c \""+command+"\""),IntPtr.Zero,IntPtr.Zero,true,0x08080004,IntPtr.Zero,null,ref startup,out process));
   if(ResumeThread(process.Thread)==0xffffffff)throw new Win32Exception();
   if(WaitForSingleObject(process.Process,0xffffffff)!=0)throw new Win32Exception();
   uint code;Require(GetExitCodeProcess(process.Process,out code));return unchecked((int)code);
  } finally {
   if(initialized)DeleteProcThreadAttributeList(attributes);if(attributes!=IntPtr.Zero)Marshal.FreeHGlobal(attributes);if(jobValue!=IntPtr.Zero)Marshal.FreeHGlobal(jobValue);
   // Closing this handle kills remaining descendants, including after the root exited.
   CloseHandle(job);if(process.Thread!=IntPtr.Zero)CloseHandle(process.Thread);if(process.Process!=IntPtr.Zero)CloseHandle(process.Process);
  }
 }
}
