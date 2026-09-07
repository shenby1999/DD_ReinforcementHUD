using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using Microsoft.Win32;

class StartWithHud
{
    static string BaseDir = AppDomain.CurrentDomain.BaseDirectory;
    static string Injector = Path.Combine(BaseDir, "Injector.exe");
    static string HudDll = Path.Combine(BaseDir, "ReinforcementHudColor.dll");
    static string GamePathFile = Path.Combine(BaseDir, "game_path.txt");

    [DllImport("kernel32.dll", SetLastError=true)]
    static extern IntPtr CreateToolhelp32Snapshot(uint dwFlags, uint th32ProcessID);
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Ansi)]
    static extern bool Module32First(IntPtr hSnapshot, ref MODULEENTRY32 lpme);
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Ansi)]
    static extern bool Module32Next(IntPtr hSnapshot, ref MODULEENTRY32 lpme);
    [DllImport("kernel32.dll")]
    static extern bool CloseHandle(IntPtr h);

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Ansi)]
    struct MODULEENTRY32
    {
        public uint dwSize;
        public uint th32ModuleID;
        public uint th32ProcessID;
        public uint GlblcntUsage;
        public uint ProccntUsage;
        public IntPtr modBaseAddr;
        public uint modBaseSize;
        public IntPtr hModule;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=256)] public string szModule;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)] public string szExePath;
    }

    static int Main(string[] args)
    {
        Console.OutputEncoding = Encoding.UTF8;
        Console.WriteLine("=== Darkest Dungeon HUD Launcher ===");

        Process game = FindDarkest();
        if (game == null)
        {
            string exe = FindGameExe();
            if (exe == null)
            {
                Console.WriteLine("Cannot find Darkest.exe automatically.");
                Console.WriteLine("Please create a file named game_path.txt next to this exe with the full path to Darkest.exe.");
                return 1;
            }

            string argLine = args.Length > 0 ? string.Join(" ", args.Select(a => "\"" + a + "\"")) : "";
            Console.WriteLine("Starting Darkest.exe " + argLine);
            var psi = new ProcessStartInfo
            {
                FileName = exe,
                WorkingDirectory = Path.GetDirectoryName(exe),
                Arguments = argLine,
                UseShellExecute = false
            };
            game = Process.Start(psi);
            if (game == null) { Console.WriteLine("Failed to start game."); return 1; }
        }
        else
        {
            Console.WriteLine("Found running Darkest process: PID " + game.Id);
        }

        Console.WriteLine("Waiting for game window...");
        game.Refresh();
        for (int i = 0; i < 60; i++)
        {
            try
            {
                game.Refresh();
                if (game.MainWindowHandle != IntPtr.Zero && game.Responding) break;
            }
            catch { }
            Thread.Sleep(500);
        }
        Thread.Sleep(4000);

        if (IsHudLoaded((uint)game.Id))
        {
            Console.WriteLine("HUD already injected, skipping to avoid duplicates.");
            return 0;
        }

        Console.WriteLine("Injecting HUD into PID " + game.Id);
        var pinfo = new ProcessStartInfo
        {
            FileName = Injector,
            Arguments = "\"" + game.Id + "\" \"" + HudDll + "\"",
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };
        using (var p = Process.Start(pinfo))
        {
            string stdout = p.StandardOutput.ReadToEnd();
            string stderr = p.StandardError.ReadToEnd();
            p.WaitForExit();
            Console.WriteLine(stdout);
            if (!string.IsNullOrWhiteSpace(stderr)) Console.WriteLine(stderr);
            Console.WriteLine("Injector exit code: " + p.ExitCode);
        }
        Console.WriteLine("HUD launch finished.");
        return 0;
    }

    // If this package is unpacked directly inside the Darkest Dungeon game
    // root folder (the common portable setup), locate the game beside it.
    static string FindLocalDarkest()
    {
        // Game root layout: .../DarkestDungeon/_windows/win64/Darkest.exe
        string rootLayout = Path.Combine(BaseDir, "_windows", "win64", "Darkest.exe");
        if (File.Exists(rootLayout)) return rootLayout;

        // Also cover the case where the package is unpacked in the same
        // folder as Darkest.exe itself.
        string sameDir = Path.Combine(BaseDir, "Darkest.exe");
        if (File.Exists(sameDir)) return sameDir;

        return null;
    }

    static string FindGameExe()
    {
        // 1. If users put this package in the Darkest Dungeon game root folder,
        //    use that location first. This is the most common portable setup.
        string localExe = FindLocalDarkest();
        if (localExe != null)
        {
            TryWriteGamePath(localExe);
            return localExe;
        }

        // 2. Use existing game_path.txt if present and valid.
        if (File.Exists(GamePathFile))
        {
            string line = File.ReadAllLines(GamePathFile).FirstOrDefault(x => !string.IsNullOrWhiteSpace(x));
            if (!string.IsNullOrWhiteSpace(line))
            {
                string p = line.Trim().Trim('"');
                if (File.Exists(p)) return p;
            }
        }

        // 3. Auto-detect Steam install path from registry.
        var steamRoots = new System.Collections.Generic.List<string>();
        string registryPath = GetRegistrySteamPath();
        if (!string.IsNullOrWhiteSpace(registryPath))
        {
            AddLibraryFolders(registryPath, steamRoots);
        }

        // 4. Include common defaults as fallback.
        string[] fallbackRoots = new string[]
        {
            @"C:\Program Files (x86)\Steam",
            @"C:\Program Files\Steam",
            @"D:\Softwares\steam",
            @"D:\Steam",
            @"D:\SteamLibrary",
            @"E:\Steam",
            @"E:\SteamLibrary"
        };
        foreach (var f in fallbackRoots) AddLibraryFolders(f, steamRoots);

        foreach (var root in steamRoots)
        {
            string exe = TryGame(root);
            if (exe != null)
            {
                TryWriteGamePath(exe);
                return exe;
            }
        }
        return null;
    }

    static string GetRegistrySteamPath()
    {
        try
        {
            using (var key = Registry.CurrentUser.OpenSubKey(@"Software\Valve\Steam"))
            {
                if (key != null)
                {
                    string p = key.GetValue("SteamPath") as string;
                    if (!string.IsNullOrWhiteSpace(p)) return p;
                }
            }
        }
        catch { }
        try
        {
            using (var key = Registry.LocalMachine.OpenSubKey(@"Software\WOW6432Node\Valve\Steam"))
            {
                if (key != null)
                {
                    string p = key.GetValue("InstallPath") as string;
                    if (!string.IsNullOrWhiteSpace(p)) return p;
                }
            }
        }
        catch { }
        try
        {
            using (var key = Registry.LocalMachine.OpenSubKey(@"Software\Valve\Steam"))
            {
                if (key != null)
                {
                    string p = key.GetValue("InstallPath") as string;
                    if (!string.IsNullOrWhiteSpace(p)) return p;
                }
            }
        }
        catch { }
        return null;
    }

    static void AddLibraryFolders(string steamRoot, System.Collections.Generic.List<string> roots)
    {
        if (string.IsNullOrWhiteSpace(steamRoot)) return;
        roots.Add(steamRoot);

        // Parse libraryfolders.vdf for additional Steam libraries.
        string vdf = Path.Combine(steamRoot, "steamapps", "libraryfolders.vdf");
        if (!File.Exists(vdf)) return;
        try
        {
            foreach (var line in File.ReadAllLines(vdf))
            {
                var m = Regex.Match(line, "\"path\"\\s+\"(.+?)\"");
                if (m.Success)
                {
                    string lib = m.Groups[1].Value.Replace("\\\\", "\\");
                    if (!string.IsNullOrWhiteSpace(lib) && !roots.Contains(lib)) roots.Add(lib);
                }
            }
        }
        catch { }
    }

    static string TryGame(string steamRoot)
    {
        string exe = Path.Combine(steamRoot, "steamapps", "common", "DarkestDungeon", "_windows", "win64", "Darkest.exe");
        return File.Exists(exe) ? exe : null;
    }

    static void TryWriteGamePath(string exe)
    {
        try
        {
            File.WriteAllText(GamePathFile, exe + Environment.NewLine);
        }
        catch { }
    }

    static bool IsHudLoaded(uint pid)
    {
        IntPtr snap = CreateToolhelp32Snapshot(0x00000008 /*TH32CS_SNAPMODULE*/, pid);
        if (snap == IntPtr.Zero || snap == new IntPtr(-1)) return false;
        try
        {
            MODULEENTRY32 me = new MODULEENTRY32();
            me.dwSize = (uint)Marshal.SizeOf(typeof(MODULEENTRY32));
            if (!Module32First(snap, ref me)) return false;
            do
            {
                string name = (me.szModule ?? "").ToLowerInvariant();
                if (name.Contains("reinforcementhud") ||
                    name.Contains("glhud") ||
                    name.Contains("gldigit") ||
                    name.Contains("hudoverlay") ||
                    name.Contains("hudsolid") ||
                    name.Contains("testinject"))
                    return true;
            } while (Module32Next(snap, ref me));
        }
        finally { CloseHandle(snap); }
        return false;
    }

    static Process FindDarkest()
    {
        foreach (var p in Process.GetProcessesByName("Darkest"))
        {
            try { if (p.ProcessName == "Darkest") return p; } catch { }
        }
        return null;
    }
}
