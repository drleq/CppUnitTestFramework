import * as vscode from 'vscode';
import { TestHub, testExplorerExtensionId } from 'vscode-test-adapter-api';
import { Adapter } from './CppUnitTestFramework/Adapter';

//------------------------------------------------------------------------------------------------------------

const _registeredAdapters = new Map<vscode.WorkspaceFolder, Adapter>();

//------------------------------------------------------------------------------------------------------------

function _getTestExplorerUIExtension(): vscode.Extension<TestHub> | undefined {
    const testExplorerExtension = vscode.extensions.getExtension<TestHub>(testExplorerExtensionId);
	if (!testExplorerExtension) {
        // The Test Explorer UI extension isn't available.  Nothing we can do.
        return undefined;
    }

    return testExplorerExtension;
}

//------------------------------------------------------------------------------------------------------------

function _onWorkspaceFolderChanged(event: vscode.WorkspaceFoldersChangeEvent) {
    const testExplorerExtension = _getTestExplorerUIExtension();
    if (!testExplorerExtension) {
        return;
    }

    for (const workspaceFolder of event.removed) {
        const adapter = _registeredAdapters.get(workspaceFolder);
        if (adapter) {
            testExplorerExtension.exports.unregisterTestAdapter(adapter);
            _registeredAdapters.delete(workspaceFolder);
        }
    }

    for (const workspaceFolder of event.added) {
        const adapter = new Adapter(workspaceFolder);
        _registeredAdapters.set(workspaceFolder, adapter);
        testExplorerExtension.exports.registerTestAdapter(adapter);
    }
}

//------------------------------------------------------------------------------------------------------------

export async function activate(context: vscode.ExtensionContext) {
	const testExplorerExtension = _getTestExplorerUIExtension();
	if (!testExplorerExtension) {
        // The Test Explorer UI extension isn't available.  Nothing we can do.
        return;
    }

    if (!testExplorerExtension.isActive) {
        // The Test Explorer UI extension is available but isn't active.  Turn it on.
        await testExplorerExtension.activate();
    }

    // Listen for future workspace changes
    vscode.workspace.onDidChangeWorkspaceFolders(_onWorkspaceFolderChanged);
    if (vscode.workspace.workspaceFolders) {
        // Create a CppUnitTestFrameworkAdapter for each current workspace, registering them with the
        // Test Explorer UI extension.
        for (const workspaceFolder of vscode.workspace.workspaceFolders) {
            const adapter = new Adapter(workspaceFolder);
            _registeredAdapters.set(workspaceFolder, adapter);
            testExplorerExtension.exports.registerTestAdapter(adapter);
        }
    }
}