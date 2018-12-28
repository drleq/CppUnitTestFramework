import * as childProcess from 'child_process';
import * as vscode from 'vscode';
import { DisposableBase } from './DisposableBase';

export class AsyncExec extends DisposableBase {
    private readonly _exitEmitter = new vscode.EventEmitter<number>();
    private readonly _errorEmitter = new vscode.EventEmitter<Error>();
    private readonly _stdoutLineEmitter = new vscode.EventEmitter<string>();
    private _process?: childProcess.ChildProcess = undefined;

    private _incompleteLine?: string = undefined;

    //--------------------------------------------------------------------------------------------------------

    public get onExit(): vscode.Event<number> {
        return this._exitEmitter.event;
    }
    public get onError(): vscode.Event<Error> {
        return this._errorEmitter.event;
    }
    public get onStdoutLine(): vscode.Event<string> {
        return this._stdoutLineEmitter.event;
    }

    //--------------------------------------------------------------------------------------------------------

    public start(
        executable: string,
        args: string[],
        workingDirectory: string,
        environment: NodeJS.ProcessEnv
    ) {
        if (this._process) {
            throw new Error('Process is already started');
        }

        const spawnConfig: childProcess.SpawnOptions = {
            cwd: workingDirectory,
            env: { ...process.env, ...environment }
        };

        this._process = childProcess.spawn(executable, args, spawnConfig);
        this._process.on('exit', (code, signal) => { this._onExit(code, signal); });
        this._process.on('error', (error) => { this._onError(error) });
        this._process.stdout.on('data', (chunk) => { this._onChunk(chunk); });
    }

    //--------------------------------------------------------------------------------------------------------

    public dispose() {
        if (this._process) {
            if (!this._process.killed) {
                this._process.kill();
            }
            this._process = undefined;
        }

        super.dispose();
    }

    //--------------------------------------------------------------------------------------------------------

    private _onExit(code: number | null, signal: string | null) {
        this._flushIncompleteLine();

        if (code === null) {
            code = 0;
        }
        this._exitEmitter.fire(code);
    }

    private _onError(error: Error) {
        this._flushIncompleteLine();
        this._errorEmitter.fire(error);
    }

    private _onChunk(chunk: any) {
        // Split the chunk into lines.
        const lines = chunk.toString().split(/[\r\n]+/);

        // Append the first line from the chunk with the [this._incompleteLine], if one exists.
        if (this._incompleteLine) {
            this._incompleteLine += lines.shift();
        }

        // If we have at least one complete line then output all complete lines.
        if (lines.length > 0) {
            // [this._incompleteLine] must be complete.
            if (this._incompleteLine) {
                this._stdoutLineEmitter.fire(this._incompleteLine);
            }

            // Set [this._incompleteLine] to the last line in the chunk.  We don't know for sure that
            // it's complete yet.
            this._incompleteLine = lines.pop();

            // Output remaining complete lines.
            for (let line of lines) {
                this._stdoutLineEmitter.fire(line);
            }
        }
    }

    private _flushIncompleteLine() {
        if (this._incompleteLine) {
            this._stdoutLineEmitter.fire(this._incompleteLine);
            this._incompleteLine = undefined;
        }
    }
}