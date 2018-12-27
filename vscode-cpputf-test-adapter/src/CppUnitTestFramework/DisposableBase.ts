import * as vscode from 'vscode';
import { DisposableSet } from './DisposableSet';

export abstract class DisposableBase implements vscode.Disposable {
    private readonly _disposables = new DisposableSet();

    protected track(disposable: vscode.Disposable) {
        this._disposables.add(disposable);
    }

    protected untrack(disposable: vscode.Disposable) {
        this._disposables.remove(disposable);
    }

    public dispose() {
        this._disposables.dispose();
    }
}