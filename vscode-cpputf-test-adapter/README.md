# CppUnitTestFramework Test Adapter
This extension is a Visual Studio Code Test Adapter for the [Test Explorer UI](https://marketplace.visualstudio.com/items?itemName=hbenl.vscode-test-explorer) extension.  It integrates with the [CppUnitTestFramework](https://github.com/drleq/CppUnitTestFramework) unit testing library.

# Configuration
Each workspace must be configured in order for the extension to work correctly.  The following settings are available:

Property                                | Description
----------------------------------------|----------------------------------------------------------------------------------------------------------
`cppUnitTestFramework.executable`       | _Required_: The path to the compiled unit test executable.  Can be absolute or relative to the workspace.
`cppUnitTestFramework.workingDirectory` | _Optional_: The working directory to use when running the unit test executable.  Can be absolute or relative to the workspace.  Defaults to the executable directory.
`cppUnitTestFramework.environment`      | _Optional_: A map of additional environment variables to apply when running the unit test executable.
`cppUnitTestFramework.debugLogging`     | _Optional_: Enabled debug logging for the extension.  Defaults to `false`.

# Debugging
Currently, the only debugger supported is `gdb`.  If this is not available in your workspace then the feature will not work.