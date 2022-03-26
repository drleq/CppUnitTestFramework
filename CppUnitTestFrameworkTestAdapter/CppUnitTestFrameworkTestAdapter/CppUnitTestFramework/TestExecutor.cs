using Microsoft.VisualStudio.TestPlatform.ObjectModel;
using Microsoft.VisualStudio.TestPlatform.ObjectModel.Adapter;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;

namespace CppUnitTestFrameworkTestAdapter.CppUnitTestFramework
{
    [ExtensionUri(ExecutorUri)]
    public class TestExecutor :
        TestAdapterBase,
        ITestExecutor
    {
        public const string ExecutorUri = "executor://CppUTFTestExecutor";

        private Process m_test_run_process = null;
        
        //----------------------------------------------------------------------------------------------------

        public void Cancel() {
            lock (this) {
                if (m_test_run_process != null) {
                    m_test_run_process.Kill();
                }
            }
        }

        //----------------------------------------------------------------------------------------------------

        public void RunTests(
            IEnumerable<TestCase> tests,
            IRunContext runContext,
            IFrameworkHandle frameworkHandle
        ) {
            if (!Initialize(frameworkHandle, runContext)) {
                return;
            }

            // Group the tests by executable to minimize the number of processes
            var per_executable = tests.GroupBy(t => t.Source).ToList();

            // Run specific test cases
            foreach (var executable_group in per_executable) {
                var executable = executable_group.Key;
                RunAndParseTests(runContext, frameworkHandle, executable, executable_group);
            }
        }

        //----------------------------------------------------------------------------------------------------

        public void RunTests(
            IEnumerable<string> sources,
            IRunContext runContext,
            IFrameworkHandle frameworkHandle
        ) {
            if (!Initialize(frameworkHandle, runContext)) {
                return;
            }

            // Run all test cases from each executable
            foreach (var executable in sources) {
                // Discover the test cases
                var task = DiscoverTestsFromExecutable(executable);
                task.Wait();
                if (task.Status != TaskStatus.RanToCompletion) {
                    LogError("Failed to discover tests for " + executable);
                    continue;
                }

                // Run all test cases
                RunAndParseTests(runContext, frameworkHandle, executable, task.Result);
            }
        }

        //----------------------------------------------------------------------------------------------------

        private void RunAndParseTests(
            IRunContext run_context,
            IFrameworkHandle frameworkHandle,
            string executable,
            IEnumerable<TestCase> tests
        ) {
            var args = tests.Select(t => t.FullyQualifiedName).ToList();
            args.Insert(0, "--verbose --adapter_info");

            Task parse_output_task;
            lock (this) {
                if (m_test_run_process != null) {
                    LogInfo("Cannot run tests while other tests are active");
                    return;
                }

                if (run_context.IsBeingDebugged) {
                    m_test_run_process = StartDebugTestRun(frameworkHandle, executable, args.ToArray());
                } else { 
                    m_test_run_process = StartTestRun(executable, args.ToArray());
                }
                if (m_test_run_process == null) {
                    LogError("Failed to launch " + executable);
                }

                parse_output_task = ParseTestRunOutput(frameworkHandle, m_test_run_process, tests);
            }

            parse_output_task.Wait();
            if (parse_output_task.Status != TaskStatus.RanToCompletion) {
                LogError("Failed to discover tests for " + executable);
            }

            lock (this) {
                m_test_run_process = null;
            }
        }

        //----------------------------------------------------------------------------------------------------

        private async Task ParseTestRunOutput(
            ITestExecutionRecorder recorder,
            Process process,
            IEnumerable<TestCase> tests
        ) {
            if (!process.StartInfo.RedirectStandardOutput) {
                // A debugged process won't re-direct stdout for us while it's running.  We can't know what
                // the results are (and they may not even be accurate if the tests exited early).
                process.WaitForExit();
                return;
            }
            var stream = process.StandardOutput;

            var tests_complete = false;
            TestCase current_test = null;
            var current_message_lines = new List<string>();

            do {
                var line = await stream.ReadLineAsync();
                if (line == null) {
                    break;
                }

                if (tests_complete) {
                    // We have stopped processing tests for some reason.
                    break;
                }

                if (line.StartsWith("Test Complete:")) {
                    if (current_test == null) {
                        LogError("Unexpected test completion");
                        break;
                    }

                    // The current test has finished.  Update the status.
                    var status = line.Substring(15);
                    var outcome = (status == "passed") ? TestOutcome.Passed : TestOutcome.Failed;
                    var message = string.Join(Environment.NewLine, current_message_lines);
                    recorder.RecordResult(
                        new TestResult(current_test) {
                            Outcome = outcome,
                            ErrorMessage = message
                        }
                    );
                    LogDebug($"Test Complete: {current_test.FullyQualifiedName} ({status})");

                    current_test = null;
                    current_message_lines.Clear();
                    continue;
                }

                if (line.StartsWith("Skip:")) {
                    var test_name = line.Substring(6);

                    var test_case = tests.FirstOrDefault(t => t.FullyQualifiedName == test_name);
                    if (test_case != null) {
                        recorder.RecordResult(
                            new TestResult(test_case) { Outcome = TestOutcome.Skipped }
                        );
                    }
                    LogDebug($"Test Skipped: {test_name}");
                    continue;
                }

                if (line.StartsWith("Test:")) {
                    var test_name = line.Substring(6);
                    current_test = tests.First(t => t.FullyQualifiedName == test_name);
                    recorder.RecordStart(current_test);
                    continue;
                }

                if (line.StartsWith("Running")) {
                    // Ignore the first line.
                    continue;
                }

                if (line.StartsWith("Complete.")) {
                    // All done.
                    tests_complete = true;
                    continue;
                }

                if (line.Length != 0) {
                    if (line.StartsWith("    ")) {
                        line = line.Substring(4);
                    }

                    // Assume output from the test
                    current_message_lines.Add(line);
                    continue;
                }

                // Something has gone wrong.  Stop processing.
                tests_complete = true;
            } while (!stream.EndOfStream);
        }
    }
}
