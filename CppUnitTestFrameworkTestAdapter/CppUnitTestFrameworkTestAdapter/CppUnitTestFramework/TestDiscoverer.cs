using Microsoft.VisualStudio.TestPlatform.ObjectModel;
using Microsoft.VisualStudio.TestPlatform.ObjectModel.Adapter;
using Microsoft.VisualStudio.TestPlatform.ObjectModel.Logging;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace CppUnitTestFrameworkTestAdapter.CppUnitTestFramework
{
    [FileExtension(".exe")]
    [DefaultExecutorUri(TestExecutor.ExecutorUri)]
    public class TestDiscoverer :
        TestAdapterBase,
        ITestDiscoverer
    {
        public void DiscoverTests(
            IEnumerable<string> sources,
            IDiscoveryContext discoveryContext,
            IMessageLogger logger,
            ITestCaseDiscoverySink discoverySink
        ) {
            if (!Initialize(logger, discoveryContext)) {
                return;
            }

            foreach (var executable in sources) {
                var task = DiscoverTestsFromExecutable(executable);
                task.Wait();
                if (task.Status != TaskStatus.RanToCompletion) {
                    LogError("Failed to discover tests for " + executable);
                    continue;
                }

                foreach (var test_case in task.Result) {
                    discoverySink.SendTestCase(test_case);
                }
            }
        }
    }
}
