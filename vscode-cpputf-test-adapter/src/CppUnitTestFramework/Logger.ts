import * as vscode from 'vscode';
import { Configuration } from './Configuration';

export class Logger {
    private readonly _channel?: vscode.OutputChannel;

    constructor(configuration: Configuration) {
        if (configuration.isDebugLoggingEnabled) {
            this._channel = vscode.window.createOutputChannel("CppUnitTestFramework");
        }
    }

    public write(... parts: string[]) {
        if (this._channel) {
            this._channel.appendLine(parts.join());
        }
    }
}