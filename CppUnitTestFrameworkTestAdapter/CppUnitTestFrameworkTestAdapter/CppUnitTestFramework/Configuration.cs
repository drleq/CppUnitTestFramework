using Microsoft.VisualStudio.TestPlatform.ObjectModel.Logging;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;

namespace CppUnitTestFrameworkTestAdapter.CppUnitTestFramework
{
    public class Configuration
    {
        public bool Enabled { get; private set; }
        public string WorkingDirectory00 { get; private set; }
        public Dictionary<string, string> Environment { get; private set; }
        public bool DebugLogging { get; private set; }

        //----------------------------------------------------------------------------------------------------

        public Configuration() {
            Enabled = true;
            WorkingDirectory00 = null;
            Environment = new Dictionary<string, string>();
            DebugLogging = true;
        }

        //----------------------------------------------------------------------------------------------------

        public Configuration(string settings_xml)
            : this()
        {
            var doc = new XmlDocument();
            doc.LoadXml(settings_xml);

            var root = doc.SelectSingleNode("RunSettings/CppUnitTestFramework");
            if (root == null) {
                return;
            }

            // CppUnitTestFrameworkTestAdaptor.Enabled
            var enabled00 = root.SelectSingleNode("Enabled");
            if (enabled00 != null && bool.TryParse(enabled00.InnerText, out bool value)) {
                Enabled = value;
            } else {
                Enabled = true;
            }

            // CppUnitTestFrameworkTestAdaptor.WorkingDirectory
            var working_directory00 = root.SelectSingleNode("WorkingDirectory");
            WorkingDirectory00 = working_directory00?.InnerText;

            // CppUnitTestFrameworkTestAdaptor.DebugLogging
            var debug_logging00 = root.SelectSingleNode("DebugLogging");
            if (debug_logging00 != null && bool.TryParse(debug_logging00.InnerText, out bool value2)) {
                DebugLogging = value2;
            } else {
                DebugLogging = true;
            }

            // CppUnitTestFrameworkTestAdaptor.Environment
            var environment00 = root.SelectSingleNode("Environment");
            Environment = new Dictionary<string, string>();
            if (environment00 != null) {
                foreach (XmlNode entry in environment00.ChildNodes) {
                    Environment.Add(entry.Name, entry.InnerText);
                }
            }
        }
    }
}
