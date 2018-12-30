using Microsoft.VisualStudio.TestPlatform.ObjectModel;
using Microsoft.VisualStudio.TestPlatform.ObjectModel.Adapter;
using Microsoft.VisualStudio.TestPlatform.ObjectModel.Logging;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;

namespace CppUnitTestFrameworkTestAdapter.CppUnitTestFramework
{
    public class TestAdapterBase
    {
        private IMessageLogger m_logger = null;
        private string m_solution_directory00 = null;

        protected Configuration Config { get; private set; }

        //----------------------------------------------------------------------------------------------------

        protected bool Initialize(IMessageLogger logger, IDiscoveryContext context) {
            m_logger = logger;

            Config = new Configuration(context.RunSettings.SettingsXml);
            LogDebug($"Config.Enabled            = {Config.Enabled}");
            LogDebug($"Config.WorkingDirectory00 = {Config.WorkingDirectory00}");
            LogDebug($"Config.Environment:");
            foreach (var kvp in Config.Environment) {
                LogDebug($"    {kvp.Key} = {kvp.Value}");
            }

            if (context is IRunContext run_context00) {
                m_solution_directory00 = run_context00.SolutionDirectory;
                LogDebug($"Solution directory = {m_solution_directory00}");
            }

            if (!Config.Enabled) {
                LogDebug("CppUnitTestFramework test adapter is disabled.");
                return false;
            }

            return true;
        }

        //----------------------------------------------------------------------------------------------------

        #region Logging
        protected void LogInfo(params string[] text) {
            if (m_logger != null) {
                m_logger.SendMessage(TestMessageLevel.Informational, "[CppUTF] " + string.Join("", text));
            }
        }

        //----------------------------------------------------------------------------------------------------

        protected void LogWarn(params string[] text) {
            if (m_logger != null) {
                m_logger.SendMessage(TestMessageLevel.Warning, "[CppUTF] " + string.Join("", text));
            }
        }

        //----------------------------------------------------------------------------------------------------

        protected void LogError(params string[] text) {
            if (m_logger != null) {
                m_logger.SendMessage(TestMessageLevel.Error, "[CppUTF] " + string.Join("", text));
            }
        }

        //----------------------------------------------------------------------------------------------------

        protected void LogDebug(params string[] text) {
            if (Config.DebugLogging) {
                LogInfo(text);
            }
        }
        #endregion

        //----------------------------------------------------------------------------------------------------

        protected Process StartTestRun(string executable, params string[] args) {
            var process = new Process();
            process.StartInfo.CreateNoWindow = true;
            process.StartInfo.RedirectStandardOutput = true;
            process.StartInfo.UseShellExecute = false;
            process.StartInfo.FileName = executable;
            process.StartInfo.Arguments = string.Join(" ", args);
            process.StartInfo.WorkingDirectory = GetWorkingDirectory(executable);
            foreach (var kvp in Config.Environment) {
                process.StartInfo.Environment.Add(kvp);
            }

            LogDebug($"Starting {executable} {process.StartInfo.Arguments}");

            if (!process.Start()) {
                return null;
            }
            return process;
        }

        //----------------------------------------------------------------------------------------------------

        private string GetWorkingDirectory(string executable) {
            if (Config.WorkingDirectory00 == null) {
                // No working directory in the config.  Return either the test run directory or the
                // folder of the executable.
                return m_solution_directory00 ?? Path.GetDirectoryName(executable);
            }

            if (Path.IsPathRooted(Config.WorkingDirectory00)) {
                // The config has provided an absolute path.  Just use that.
                return Config.WorkingDirectory00;
            }

            // The config has provided a relative path.  Combine it with either the test run directory,
            // the solution directory or (as a last resort) the executable directory.
            if (m_solution_directory00 != null) {
                return Path.Combine(m_solution_directory00, Config.WorkingDirectory00);
            }
            return Path.Combine(Path.GetDirectoryName(executable), Config.WorkingDirectory00);
        }

        //----------------------------------------------------------------------------------------------------

        #region Test Discovery
        protected async Task<List<TestCase>> DiscoverTestsFromExecutable(string executable) {
            var tests = new List<TestCase>();

            var process = StartTestRun(executable, "--discover_tests", "--adapter_info");
            if (process == null) {
                LogError("Failed to launch " + executable);
                return tests;
            }

            try {
                return await ParseTestDiscovery(executable, process.StandardOutput);
            } catch (Exception e) {
                LogError("Failed to parse: " + e.Message);
                return tests;
            }
        }

        //----------------------------------------------------------------------------------------------------

        private async Task<List<TestCase>> ParseTestDiscovery(
            string executable,
            StreamReader reader
        ) {
            var NameSplit = new string[] { "::" };
            var tests = new List<TestCase>();

            do {
                var line = await reader.ReadLineAsync();
                if (line == null) {
                    break;
                }

                var parts = line.Split(',');
                if (parts.Length != 3) {
                    LogWarn("Failed to split line: " + line);
                    continue;
                }

                var name_parts = parts[0].Split(NameSplit, StringSplitOptions.None);
                if (name_parts.Length != 2) {
                    LogWarn("Failed to split line: " + parts[0]);
                    continue;
                }
                var fixture_name = name_parts[0];
                var test_name = name_parts[1];

                var source_file = parts[1];
                int source_line;
                if (!int.TryParse(parts[2], out source_line)) {
                    LogWarn("Failed to parse line number: " + parts[2]);
                    continue;
                }

                var test = new TestCase {
                    Source = executable,
                    ExecutorUri = new Uri(TestExecutor.ExecutorUri),
                    FullyQualifiedName = parts[0],
                    CodeFilePath = source_file,
                    LineNumber = source_line
                };
                test.Traits.Add(new Trait("Fixture", fixture_name));
                tests.Add(test);

                LogDebug($"Discovered {test.FullyQualifiedName}");
            } while (!reader.EndOfStream);

            return tests;
        }
        #endregion
    }
}
