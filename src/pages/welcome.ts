"use strict";
import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
import opn = require('open');
import * as Application from '../application';

export class WelcomePage implements vscode.Disposable {

    protected currentPanel: vscode.WebviewPanel | undefined = undefined;

    public dispose(): void {
    }

    public async openWelcomePage(context: vscode.ExtensionContext) {
        console.log('debugger:WelcomePage.openWelcomePage');

        // Prepare
        let contentPath = path.join(context.extensionPath, 'out', 'content', 'pages', 'welcome');
        let columnToShowIn = vscode.window.activeTextEditor
                                ? vscode.window.activeTextEditor.viewColumn
                                : undefined;

        // Open or create panel?
        if (this.currentPanel) {
            // Open
            this.currentPanel.reveal(columnToShowIn);

        } else {
            // Create
            this.currentPanel = vscode.window.createWebviewPanel(
                'webpage',
                'Atari Dev Studio Welcome',
                columnToShowIn || vscode.ViewColumn.One,
                {
                    enableScripts: true,
                    localResourceRoots: [vscode.Uri.file(contentPath)]
                }
            );

            // Content
            let startPagePath = vscode.Uri.file(path.join(contentPath.toString(), 'index.html'));
            let content = fs.readFileSync(startPagePath.fsPath, 'utf8');
            let nonce = this.getNonce();
            
            // Script
            let scriptJsPath = vscode.Uri.file(path.join(contentPath.toString(), 'script.js'));
            let scriptJsUri = scriptJsPath.with({ scheme: 'vscode-resource' });

            // Style
            let styleCssPath = vscode.Uri.file(path.join(contentPath.toString(), 'style.css'));
            let styleCssUri = styleCssPath.with({ scheme: 'vscode-resource' });

            // Update tags in content
            content = this.replaceContentTag(content, "APPDISPLAYNAME", Application.DisplayName);
            content = this.replaceContentTag(content, "APPDESCRIPTION", Application.Description);
            content = this.replaceContentTag(content, "APPVERSION", Application.Version);
            content = this.replaceContentTag(content, "NONCE", nonce);
            content = this.replaceContentTag(content, "SCRIPTJSURI", scriptJsUri);
            content = this.replaceContentTag(content, "STYLECSSURI", styleCssUri);

            // Set
            this.currentPanel.webview.html = content;
        }

        // Capture command messages
        this.currentPanel.webview.onDidReceiveMessage(
            message => {
                console.log(`bbFeature.openWelcomePage.command.${message.command}`);
                switch (message.command) {
                    case 'openNewFile':
                        this.openNewFileDocument("bB");
                        return;

                    case 'openFolder':
                        const options: vscode.OpenDialogOptions = {
                            canSelectFolders: true,
                            canSelectMany: false,
                            openLabel: 'Open Folder'
                        };
                        vscode.window.showOpenDialog(options).then(async folderUri => {
                            if (folderUri && folderUri[0]) {
                                console.log(`- OpenFolder: ${folderUri[0].fsPath}`);
                                await vscode.commands.executeCommand('vscode.openFolder', folderUri[0], false);
                            }
                        });                  
                        return;

                    case 'openDPCKernalTemplate':
                        let dpcTemplatePath = vscode.Uri.file(path.join(contentPath.toString(), 'templates', 'DPCKernel.bas'));
                        let dpcContent = fs.readFileSync(dpcTemplatePath.fsPath, 'utf8');
                        this.openNewFileDocument("bB", dpcContent);
                        return;

                    case 'openMultispriteKernalTemplate':
                        let multispriteTemplatePath = vscode.Uri.file(path.join(contentPath.toString(), 'templates', 'MultispriteKernel.bas'));
                        let multispriteContent = fs.readFileSync(multispriteTemplatePath.fsPath, 'utf8');
                        this.openNewFileDocument("bB", multispriteContent);
                        return;

                    case 'openStandardKernalTemplate':
                        let standardTemplatePath = vscode.Uri.file(path.join(contentPath.toString(), 'templates', 'StandardKernel.bas'));
                        let standardContent = fs.readFileSync(standardTemplatePath.fsPath, 'utf8');
                        this.openNewFileDocument("bB", standardContent);
                        return;

                    case 'openRandomTerrainPage':
                        this.openRandomTerrainPage();                   
                        return;

                    case 'openBatariBasicForum':
                        this.openBatariBasicForum();
                        return;
                }
            }
        );

        // Capture dispose
        this.currentPanel.onDidDispose(
            () => {
                this.currentPanel = undefined;
            },
            null
        );
    }

    private getNonce() {
        let text = '';
        const possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
        for (let i = 0; i < 32; i++) {
            text += possible.charAt(Math.floor(Math.random() * possible.length));
        }
        return text;
    }

    private replaceContentTag(content: string, tag: string, tagContent: any) : string
    {
        tag = `%${tag}%`;
        return content.replace(new RegExp(tag, 'g'), tagContent);
    }

    private openNewFileDocument(language: string, content: string = '') {
        vscode.workspace.openTextDocument({language: `${language}`, content: content}).then(doc => {
            // Open
            vscode.window.showTextDocument(doc);
        });
    }

    public openBatariBasicForum() {
        console.log('debugger:bBFeature.openBatariBasicForum');

        this.openUrl("http://atariage.com/forums/forum/65-batari-basic/");
    }

    public openRandomTerrainPage() {
        console.log('debugger:bBFeature.openRandomTerrainPage');

        this.openUrl("http://www.randomterrain.com/atari-2600-memories-batari-basic-commands.html");
    }

    public async openUrl(uri: string) {
        try {
            //let options:
            opn(uri); 
        }
        catch {}
    }

}