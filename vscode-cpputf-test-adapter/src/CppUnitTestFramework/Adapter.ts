import * as childProcess from 'child_process';
import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';
import * as vscode from 'vscode';
import { TestAdapter, TestSuiteEvent, TestEvent, TestSuiteInfo, TestInfo } from "vscode-test-adapter-api";
import { DisposableBase } from './DisposableBase';
import { Configuration } from './Configuration';
import { promisify } from 'util';

//------------------------------------------------------------------------------------------------------------

interface TestConfiguration {
    executable: string;
    environment: NodeJS.ProcessEnv;
    workingDirectory: string;
};

//------------------------------------------------------------------------------------------------------------

export class Adapter extends DisposableBase implements TestAdapter {
    private readonly _testStatesEmitter = new vscode.EventEmitter<TestSuiteEvent | TestEvent>();
    private readonly _reloadEmitter = new vscode.EventEmitter<void>();
    private readonly _autorunEmitter = new vscode.EventEmitter<void>();    

    private readonly _workspaceFolder: vscode.WorkspaceFolder;
    private readonly _configuration: Configuration;

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
        //this.track(this._configuration.onChanged(this._onConfigurationChanged, this));
    }

    //----------------------------------------------------------------------------------------------------

    public async load() : Promise<TestSuiteInfo | undefined> {
        return await this._discoverTests();
    }

    public async run(testInfo: TestSuiteInfo | TestInfo) : Promise<void> {
        return await this._runTests(testInfo);
    }

    public async debug(testInfo: TestSuiteInfo | TestInfo) : Promise<void> {
        return await this._debugTests(testInfo);
    }

    public cancel() : void {
        console.log("cancel");
    }

    //----------------------------------------------------------------------------------------------------

    // private _onConfigurationChanged() : void {
    //     this._reloadEmitter.fire();
    // }

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

        return {
            executable: executable,
            environment: environment,
            workingDirectory: workingDirectory
        };
    }

    //----------------------------------------------------------------------------------------------------

    private async _discoverTests() : Promise<TestSuiteInfo | undefined> {
        const testConfig = this._getTestConfig();
        if (!testConfig) {
            return undefined;
        }

        let discoveryOutput: string;

        try {
            const execFileAsync = promisify(childProcess.execFile);
            const execConfig: childProcess.ExecFileOptions = {
                cwd: testConfig.workingDirectory,
                env: { ...process.env, ...testConfig.environment }
            };

            const output = await execFileAsync(
                testConfig.executable,
                [ '--discover_tests' ],
                execConfig
            );
            discoveryOutput = output.stdout;
        } catch(error) {
            console.log(error);
            return undefined;
        }

        const lines = discoveryOutput.split(/[\r\n]+/);
        const testSuite: TestSuiteInfo = {
            type: 'suite',
            id: 'AllFixtures',
            label: 'AllFixtures',
            children: []
        };
        for (const line of lines) {
            if (line.length == 0) {
                continue;
            }

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
                file: sourceFile,
                line: Number(sourceLine)
            });
        }

        return testSuite;
    }

    //----------------------------------------------------------------------------------------------------

    private async _runTests(testInfo: TestSuiteInfo | TestInfo) : Promise<void> {
        const testConfig = this._getTestConfig();
        if (!testConfig) {
            return;
        }

        let discoveryOutput: string;

        try {
            const execFileAsync = promisify(childProcess.execFile);
            const execConfig: childProcess.ExecFileOptions = {
                cwd: testConfig.workingDirectory,
                env: { ...process.env, ...testConfig.environment }
            };

            const execArgs: string[] = [ "--verbose" ];
            if (testInfo.id != 'AllFixtures') {
                execArgs.push(testInfo.id);
            }

            const output = await execFileAsync(
                testConfig.executable,
                execArgs,
                execConfig
            );
            discoveryOutput = output.stdout;
        } catch(error) {
            console.log(error);
            return undefined;
        }

        const lines = discoveryOutput.split(/[\r\n]+/);
        lines.shift();

        while (lines.length > 0) {
            const line = <string>lines.shift();

            if (line.startsWith('Skip:')) {
                const testName = line.substr(6);
                this._updateTestStatus(testName, "skipped", "");
                continue;
            }

            if (line.startsWith('Test:')) {
                const testName = line.substr(6);

                const messageLines: string[] = [];
                while (lines[0].startsWith('    ')) {
                    const messageLine = <string>lines.shift();
                    messageLines.push(messageLine.trimLeft());
                }
                const message = messageLines.join(os.EOL);

                const status = (messageLines.length == 0) ? "passed" : "failed";

                this._updateTestStatus(testName, status, message);
                continue;
            }

            if (line.startsWith('Complete.')) {
                // All done.
                return;
            }

            // Something has gone wrong.  Stop processing.
            return;
        } 
    }

    //----------------------------------------------------------------------------------------------------

    private _debugTests(entry: TestSuiteInfo | TestInfo) : void {
        const testConfig = this._getTestConfig();
        if (!testConfig) {
            return;
        }

        const execArgs: string[] = [ "--verbose" ];
        if (entry.id != 'AllFixtures') {
            execArgs.push(entry.id);
        }

        // Build a DebugConfiguration that runs the requested tests through the C++ debugger.
        const debugLaunchConfig: vscode.DebugConfiguration = {
            name: 'Debugging ' + entry.id,
            type: 'cppdbg',
            request: 'launch',
            program: testConfig.executable,
            args: execArgs,
            cwd: testConfig.workingDirectory,
            env: testConfig.environment,
            externalConsole: false,
            MIMode: 'gdb',
            setupCommands: [
                {
                    description: "Enable pretty-printing for gdb",
                    text: "-enable-pretty-printing",
                    ignoreFailures: true
                }
            ]
        };

        vscode.debug.startDebugging(this._workspaceFolder, debugLaunchConfig);
        return;
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