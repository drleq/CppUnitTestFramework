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
            DebugLogging = false;
        }

        //----------------------------------------------------------------------------------------------------

        public Configuration(string settings_xml) {
            var doc = new XmlDocument();
            doc.LoadXml(settings_xml);

            // CppUnitTestFrameworkTestAdaptor.Enabled
            var enabled00 = doc.SelectSingleNode("Enabled");
            if (enabled00 != null && bool.TryParse(enabled00.InnerText, out bool value)) {
                Enabled = value;
            } else {
                Enabled = true;
            }

            // CppUnitTestFrameworkTestAdaptor.WorkingDirectory
            var working_directory00 = doc.SelectSingleNode("WorkingDirectory");
            WorkingDirectory00 = working_directory00?.InnerText;

            // CppUnitTestFrameworkTestAdaptor.DebugLogging
            var debug_logging00 = doc.SelectSingleNode("DebugLogging");
            if (debug_logging00 != null && bool.TryParse(debug_logging00.InnerText, out bool value2)) {
                DebugLogging = value2;
            } else {
                DebugLogging = false;
            }

            // CppUnitTestFrameworkTestAdaptor.Environment
            var environment00 = doc.SelectSingleNode("Environment");
            Environment = new Dictionary<string, string>();
            if (environment00 != null) {
                foreach (XmlNode entry in environment00.ChildNodes) {
                    Environment.Add(entry.Name, entry.InnerText);
                }
            }
        }
    }
}
