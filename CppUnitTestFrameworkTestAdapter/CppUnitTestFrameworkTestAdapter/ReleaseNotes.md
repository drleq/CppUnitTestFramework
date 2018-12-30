# CppUnitTestFramework Test Adapter
This extension is a Visual Studio 2017 Test Adapter that integrates with the [CppUnitTestFramework](https://github.com/drleq/CppUnitTestFramework) unit testing library.

# Configuration
Each workspace can be configured through the `.runsettings` file using a section named `<CppUnitTestFramework>`.  The following settings are available:

Property           | Description
-------------------|----------------------------------------------------------------------------------------------------------
`Enabled`          | _Optional_: Explicitly enables or disables the test adapter.  Defaults to `true`.
`WorkingDirectory` | _Optional_: The working directory to use when running the unit test executable.  Can be absolute or relative to the solution.  Defaults to the executable directory.
`Environment`      | _Optional_: A map of additional environment variables to apply when running the unit test executable.
`DebugLogging`     | _Optional_: Enabled debug logging for the extension.  Defaults to `false`.


### v0.3.0
  - Initial implementation.  Starting at v0.3.0 to match VSCode test adapter.