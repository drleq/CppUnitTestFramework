import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';
import * as vscode from 'vscode';
import { TestAdapter, TestSuiteEvent, TestEvent, TestSuiteInfo, TestLoadStartedEvent, TestLoadFinishedEvent } from "vscode-test-adapter-api";
import { AsyncExec } from './AsyncExec';
import { Configuration } from './Configuration';
import { DisposableBase } from './DisposableBase';
import { Logger } from './Logger';

//------------------------------------------------------------------------------------------------------------

interface TestConfiguration {
    executable: string;
    environment: NodeJS.ProcessEnv;
    workingDirectory: string;
    buildDirectory: string | undefined;
};

//------------------------------------------------------------------------------------------------------------

export class Adapter extends DisposableBase implements TestAdapter {
    private readonly _testLoadEmitter = new vscode.EventEmitter<TestLoadStartedEvent | TestLoadFinishedEvent>();
    private readonly _testStatesEmitter = new vscode.EventEmitter<TestSuiteEvent | TestEvent>();
    private readonly _reloadEmitter = new vscode.EventEmitter<void>();
    private readonly _autorunEmitter = new vscode.EventEmitter<void>();

    private readonly _workspaceFolder: vscode.WorkspaceFolder;
    private readonly _configuration: Configuration;
    private readonly _logger: Logger;

    private _executableWatcher?: vscode.FileSystemWatcher = undefined;
    private _testRun?: AsyncExec = undefined;

    //----------------------------------------------------------------------------------------------------

    get testStates() : vscode.Event<TestSuiteEvent | TestEvent> {
        return this._testStatesEmitter.event;
    }

    get reload(): vscode.Event<void> {
        return this._reloadEmitter.event;
    }

    get autorun(): vscode.Event<void> {
        return this._autorunEmitter.event;
    }

    get workspaceFolder(): vscode.WorkspaceFolder {
        return this._workspaceFolder;
    }

    //----------------------------------------------------------------------------------------------------

    constructor(
        workspaceFolder: vscode.WorkspaceFolder
    ) {
        super();

        this._workspaceFolder = workspaceFolder;
        this.track(this._configuration = new Configuration(workspaceFolder));
        this.track(this._configuration.onChanged(this._onConfigurationChanged, this));
        this._onConfigurationChanged();

        this._logger = new Logger(this._configuration);
        this._logger.write('Creating adapter for ' + workspaceFolder.name);
    }

    //----------------------------------------------------------------------------------------------------

    public async load() : Promise<void> {
        return await this._discoverTests();
    }

    public get tests() : vscode.Event<TestLoadStartedEvent | TestLoadFinishedEvent> {
        return this._testLoadEmitter.event;
    }

    public async run(testIds: string[]) : Promise<void> {
        return await this._runTests(testIds);
    }

    public async debug(testIds: string[]) : Promise<void> {
        return await this._debugTests(testIds);
    }

    public cancel() : void {
        if (this._testRun) {
            this.untrackAndDispose(this._testRun);
            this._testRun = undefined;
        }
    }

    //----------------------------------------------------------------------------------------------------

    private _onConfigurationChanged() : void {
        this._reloadEmitter.fire();

        if (this._executableWatcher) {
            this.untrackAndDispose(this._executableWatcher);
            this._executableWatcher = undefined;
        }

        const testConfig = this._getTestConfig();
        if (testConfig) {
            this._executableWatcher = vscode.workspace.createFileSystemWatcher(
                testConfig.executable,
                false,
                false,
                true
            );
            this._executableWatcher.onDidCreate((uri) => { this._reloadEmitter.fire(); });
            this._executableWatcher.onDidChange((uri) => { this._autorunEmitter.fire(); });
            this.track(this._executableWatcher);
        }
    }

    //----------------------------------------------------------------------------------------------------

    private _getTestConfig(): TestConfiguration | undefined {
        let executable = this._configuration.executable;
        if (!executable) {
            return undefined;
        }
        if (!path.isAbsolute(executable)) {
            executable = path.resolve(this._workspaceFolder.uri.fsPath, executable);
        }
        if (!fs.existsSync(executable) || !fs.statSync(executable).isFile()) {
            return undefined;
        }

        let environment = this._configuration.environment;
        if (!environment) {
            environment = {};
        }

        let workingDirectory = this._configuration.workingDirectory;
        if (!workingDirectory) {
            workingDirectory = path.dirname(executable);
        }
        if (!path.isAbsolute(workingDirectory)) {
            workingDirectory = path.resolve(this._workspaceFolder.uri.fsPath, workingDirectory);
        }

        let buildDirectory = this._configuration.buildDirectory;
        if (buildDirectory) {
            if (!path.isAbsolute(buildDirectory)) {
                buildDirectory = path.resolve(this._workspaceFolder.uri.fsPath, buildDirectory);
            }
        }

        return {
            executable: executable,
            environment: environment,
            workingDirectory: workingDirectory,
            buildDirectory: buildDirectory
        };
    }

    //----------------------------------------------------------------------------------------------------

    private async _discoverTests() : Promise<void> {
        if (this._testRun) {
            // Something is already running.
            this._logger.write('Cannot discover tests while a test run is active');
            return;
        }

        const testConfig = this._getTestConfig();
        if (!testConfig) {
            this._logger.write('Failed to get the test configuration.');
            this._testLoadEmitter.fire(<TestLoadFinishedEvent>{
                errorMessage: 'Failed to get the test configuration.  Is settings.json configured?'
            });
            return;
        }

        const testSuite: TestSuiteInfo = {
            type: 'suite',
            id: 'AllFixtures',
            label: 'AllFixtures',
            children: []
        };

        const resolveSourceFile = (sourceFile: string): string => {
            // If the source file can be found (either absolute or relative) then just us it.
            if (fs.existsSync(sourceFile)) { return sourceFile; }

            // Attempt to resolve the source file with the provided build directory.
            if (testConfig.buildDirectory) {
                let fullPath = path.resolve(testConfig.buildDirectory, sourceFile);
                if (fs.existsSync(fullPath)) {
                    return fullPath;
                }
            }
            
            return sourceFile;
        };

        const handleLine = (line: string) => {
            const [fixtureTest, sourceFile, sourceLine] = line.split(',');
            const [fixture, test] = fixtureTest.split('::');
            
            let fixtureTests = <TestSuiteInfo>testSuite.children.find(
                value => value.id == fixture
            );
            if (!fixtureTests) {
                fixtureTests = {
                    type: 'suite',
                    id: fixture,
                    label: fixture,
                    children: []
                };
                testSuite.children.push(fixtureTests);
            }

            fixtureTests.children.push({
                type: 'test',
                id: fixtureTest,
                label: test,
                file: resolveSourceFile(sourceFile),
                line: Number(sourceLine) - 1
            });
        };

        return new Promise<void>((resolve, reject) => {
            this.track(this._testRun = new AsyncExec(this._logger));
            
            this._testRun.onExit((code) => {
                if (code == 0) {
                    this._logger.write('Discovered ' + testSuite.children.length + ' fixtures');
                    this._testLoadEmitter.fire(<TestLoadFinishedEvent>{
                        type: 'finished',
                        suite: testSuite
                    });
                    resolve();
                } else {
                    this._logger.write('Failed with code ' + code);
                    this._testLoadEmitter.fire(<TestLoadFinishedEvent>{
                        type: 'finished',
                        errorMessage: 'Test discovery failed.  Exited with code ' + code
                    });
                    reject(code);
                }
                this.untrack(<AsyncExec>this._testRun);
                this._testRun = undefined;
            })
            this._testRun.onError((error) => {
                this._logger.write('Failed with error ' + error.message);
                this._testLoadEmitter.fire(<TestLoadFinishedEvent>{
                    type: 'finished',
                    errorMessage: 'Test discovery failed.  ' + error
                });
                reject(error);
                this.untrack(<AsyncExec>this._testRun);
                this._testRun = undefined;
            });
            this._testRun.onStdoutLine(handleLine);

            this._logger.write('Discovering tests...');
            this._testRun.start(
                testConfig.executable,
                [ '--discover_tests', '--adapter_info' ],
                testConfig.workingDirectory,
                testConfig.environment
            );

            this._testLoadEmitter.fire(<TestLoadStartedEvent>{ type: 'started' });
        });
    }

    //----------------------------------------------------------------------------------------------------

    private async _runTests(testIds: string[]) : Promise<void> {
        if (this._testRun) {
            // Something is already running.
            this._logger.write('Cannot run tests while a test run is active');
            return;
        }

        const testConfig = this._getTestConfig();
        if (!testConfig) {
            this._logger.write('Failed to get the test configuration.');
            return;
        }

        let testsComplete: boolean = false;
        let currentTestName: string = '';
        let currentMessageLines: string[] = [];

        const handleLine = (line: string) => {
            if (testsComplete) {
                // We have stopped processing tests for some reason.
                return;
            }

            if (line.startsWith('    ')) {
                // A message line for the current test.
                currentMessageLines.push(line.trimLeft());
                return;
            }

            if (line.startsWith('Test Complete:')) {
                if (currentTestName.length == 0) {
                    // Unexpected test completion
                    testsComplete = true;
                    return;
                }

                // The current test has finished.  Update the status.
                const passed = line.substr(15) == "passed";
                const status = passed ? "passed" : "failed";
                const message = currentMessageLines.join(os.EOL);
                this._updateTestStatus(currentTestName, status, message);

                currentTestName = '';
                currentMessageLines = [];
                return;
            }

            if (currentTestName.length != 0) {
                // The current test has finished unexpectedly.
                testsComplete = true;
                return;
            }

            if (line.startsWith('Skip:')) {
                const testName = line.substr(6);
                this._updateTestStatus(testName, "skipped", "");
                return;
            }

            if (line.startsWith('Test:')) {
                currentTestName = line.substr(6);
                this._updateTestStatus(currentTestName, "running", '');
                return;
            }

            if (line.startsWith('Running')) {
                // Ignore the first line.
                return;
            }
            
            if (line.startsWith('Complete.')) {
                // All done.
                testsComplete = true;
                return;
            }

            // Something has gone wrong.  Stop processing.
            testsComplete = true;
            return;
        };

        return new Promise<void>((resolve, reject) => {
            this.track(this._testRun = new AsyncExec(this._logger));
            
            this._testRun.onExit((code) => {
                if (code == 0) {
                    this._logger.write('Done.');
                    resolve();
                } else {
                    this._logger.write('Failed with code ' + code);
                    reject(code);
                }
                this.untrack(<AsyncExec>this._testRun);
                this._testRun = undefined;
            })
            this._testRun.onError((error) => {
                this._logger.write('Failed with error ' + error.message);
                reject(error);
                this.untrack(<AsyncExec>this._testRun);
                this._testRun = undefined;
            });
            this._testRun.onStdoutLine(handleLine);

            const execArgs: string[] = [ "--verbose", "--adapter_info" ];
            execArgs.concat(testIds);

            this._logger.write('Running tests...');
            this._testRun.start(
                testConfig.executable,
                execArgs,
                testConfig.workingDirectory,
                testConfig.environment
            );
        });
    }

    //----------------------------------------------------------------------------------------------------

    private async _debugTests(testIds: string[]) : Promise<void> {
        if (this._testRun) {
            // Something is already running.
            this._logger.write('Cannot debug tests while a test run is active');
            return;
        }

        const testConfig = this._getTestConfig();
        if (!testConfig) {
            this._logger.write('Failed to get the test configuration.');
            return;
        }

        const execArgs: string[] = [ "--verbose" ];
        execArgs.concat(testIds);

        // Build a DebugConfiguration that runs the requested tests through the C++ debugger.
        const debugLaunchConfig: vscode.DebugConfiguration = {
            name: 'Debugging C++',
            type: 'cppvsdbg',
            linux: {
                type: 'cppdbg',
                MIMode: 'gdb',
                setupCommands: [
                    {
                        description: "Enable pretty-printing for gdb",
                        text: "-enable-pretty-printing",
                        ignoreFailures: true
                    }
                ]
            },
            osx: { type: 'cppdbg', MIMode: 'lldb' },
            request: 'launch',
            program: testConfig.executable,
            args: execArgs,
            cwd: testConfig.workingDirectory,
            env: testConfig.environment,
            externalConsole: false
        };

        return new Promise<void>(async (resolve, reject) => {
            this._logger.write('Debugging tests...');

            const success = await vscode.debug.startDebugging(this._workspaceFolder, debugLaunchConfig);
            const activeSession = vscode.debug.activeDebugSession;
            if (!success || !activeSession) {
                this._logger.write('Failed to start debugging session');
                reject("Failed to start debugging session");
                return;
            }

            const subscription = vscode.debug.onDidTerminateDebugSession((session) => {
                if (activeSession != session) {
                    return;
                }
                resolve();
                this.untrackAndDispose(subscription);
            });
            this.track(subscription);
        });
    }

    //----------------------------------------------------------------------------------------------------

    private _updateTestStatus(
        testId: string,
        state: "running" | "passed" | "failed" | "skipped",
        message: string
    ) : boolean {
        this._testStatesEmitter.fire({
            type: 'test',
            test: testId,
            state: state,
            message: message
        });
        return true;
    }
}