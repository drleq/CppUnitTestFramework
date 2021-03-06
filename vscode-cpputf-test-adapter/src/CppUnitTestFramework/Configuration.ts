import * as fs from 'fs';
import * as path from 'path';
import * as vscode from 'vscode';
import { DisposableBase } from './DisposableBase';

export class Configuration extends DisposableBase {
    private static readonly BaseName: string = 'cppUnitTestFramework';
    private static readonly ExecutableField: string = 'executable';
    private static readonly EnvironmentField: string = 'environment';
    private static readonly WorkingDirectoryField: string = 'workingDirectory';
    private static readonly DebugLoggingField: string = 'debugLogging';
    private static readonly BuildDirectoryField: string = 'buildDirectory';

    public readonly _onChangedEmitter = new vscode.EventEmitter<void>();

    //--------------------------------------------------------------------------------------------------------

    constructor(private readonly workspaceFolder: vscode.WorkspaceFolder) {
        super();

        this.track(vscode.workspace.onDidChangeConfiguration(this._onConfigurationChanged, this));
    }

    //--------------------------------------------------------------------------------------------------------

    public get onChanged() : vscode.Event<void> {
        return this._onChangedEmitter.event;
    }

    //--------------------------------------------------------------------------------------------------------

    get executable() : string | undefined {
        let executable = this._config.get<string>(Configuration.ExecutableField);
        if (!executable) {
            return undefined;
        }

        if (fs.existsSync(executable)) {
            // Path is absolute.
            return executable;
        }

        executable = path.resolve(this.workspaceFolder.uri.fsPath, executable);
        if (fs.existsSync(executable)) {
            // Path is relative to the ${workspaceFolder}.
            return executable;
        }

        let win32_executable = executable + '.exe';
        if (fs.existsSync(win32_executable)) {
            // Executable is for Windows.
            return win32_executable;
        }

        return undefined;
    }

    //--------------------------------------------------------------------------------------------------------

    get environment() : NodeJS.ProcessEnv | undefined {
        const environment = this._config.get<object>(Configuration.EnvironmentField);
        if (!environment) {
            return undefined;
        }

        return { ...environment };
    }

    //--------------------------------------------------------------------------------------------------------

    get workingDirectory() : string | undefined {
        const workingDirectory = this._config.get<string>(Configuration.WorkingDirectoryField);
        return workingDirectory ? workingDirectory : undefined;
    }

    //--------------------------------------------------------------------------------------------------------

    get isDebugLoggingEnabled() : boolean {
        const debugLogging = this._config.get<boolean>(Configuration.DebugLoggingField);
        return debugLogging ? true : false;
    }

    //--------------------------------------------------------------------------------------------------------

    get buildDirectory() : string | undefined {
        const buildDirectory = this._config.get<string>(Configuration.BuildDirectoryField);
        return buildDirectory ? buildDirectory : undefined;
    }

    //--------------------------------------------------------------------------------------------------------

    private get _config() : vscode.WorkspaceConfiguration {
        return vscode.workspace.getConfiguration(Configuration.BaseName, this.workspaceFolder.uri);
    }

    //--------------------------------------------------------------------------------------------------------

    private _onConfigurationChanged(event: vscode.ConfigurationChangeEvent) {
        if (event.affectsConfiguration(Configuration.BaseName, this.workspaceFolder.uri)) {
            this._onChangedEmitter.fire();
        }
    }
}