import * as fs from 'fs';
import * as path from 'path';
import * as vscode from 'vscode';
import { DisposableBase } from './DisposableBase';

export class Configuration extends DisposableBase {
    public static readonly BaseName: string = 'cppUnitTestFramework';
    public static readonly ExecutableField: string = 'executable';
    public static readonly EnvironmentField: string = 'environment';
    public static readonly WorkingDirectoryField: string = 'workingDirectory';

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
            return executable;
        }

        executable = path.resolve(this.workspaceFolder.uri.fsPath, executable);
        if (fs.existsSync(executable)) {
            return executable;
        }

        return undefined;
    }

    //--------------------------------------------------------------------------------------------------------

    get environment() : NodeJS.ProcessEnv | undefined {
        const environment = this._config.get(Configuration.EnvironmentField);
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