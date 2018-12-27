import * as vscode from 'vscode';

export class DisposableSet implements vscode.Disposable {
    private readonly _set = new Set<vscode.Disposable>();

    public add(disposable: vscode.Disposable) {
        this._set.add(disposable);
    }

    public remove(disposable: vscode.Disposable) {
        this._set.delete(disposable);
    }

    public dispose() {
        for (var disposable of this._set) {
            disposable.dispose();
        }
        this._set.clear();
    }
}