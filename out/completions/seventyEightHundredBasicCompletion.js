"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.SeventyEightHundredBasicCompletion = void 0;
const vscode = require("vscode");
const completionBase_1 = require("./completionBase");
class SeventyEightHundredBasicCompletion extends completionBase_1.CompletionBase {
    //private _keywords: vscode.CompletionItem[] = [];
    constructor() {
        super("7800basic");
    }
    RegisterAsync(context) {
        return __awaiter(this, void 0, void 0, function* () {
            // Read?
            //if (this._keywords.length <= 0) { this._keywords = await this.LoadCompletionFileAsync(context, '7800basic.md'); }
            /*         let languageProvider = vscode.languages.registerCompletionItemProvider(
                        this.Id,
                        {
                             // return list of available language methods
                             provideCompletionItems(document: vscode.TextDocument, position: vscode.Position) {
                                // initialise list with language items
                                var items:vscode.CompletionItem[] = [
                                    new vscode.CompletionItem('adjustvisible', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('alphachars', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('alphadata', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('BACKGRND', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P0C1', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P0C2', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P0C3', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P1C1', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P1C2', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P1C3', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P2C1', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P2C2', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P2C3', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P3C1', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P3C2', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P3C3', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P4C1', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P4C2', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P4C3', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P5C1', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P5C2', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P5C3', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P6C1', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P6C2', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P6C3', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P7C1', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P7C2', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('P7C3', vscode.CompletionItemKind.Constant),
                                    new vscode.CompletionItem('boxcollision', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('characterset', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('clearscreen', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('converttobcd', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('debug', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('displaymode', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('dmahole', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('doublebuffer', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('drawscreen', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('drawwait', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('drawhighscore', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('gamedifficulty', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('incbanner', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('incgraphic', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('incmapfile', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('inline', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('include', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy0up', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy0down', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy0left', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy0right', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy0any', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy0fire', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy0fire0', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy0fire1', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy1up', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy1down', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy1left', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy1right', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy1any', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy1fire', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy1fire0', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('joy1fire1', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('lockzone', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('loadmemory', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('loadrambank', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('loadrombank', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('memcopy', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('memset', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('newblock', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('peekchar', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('playsfx', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('playsong', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('plotbanner', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('plotchars', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('plotmap', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('plotmapfile', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('plotsprite', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('plotvalue', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('pokechar', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('pokeydetected', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('psound', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('reboot', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('restorescreen', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('savememory', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('speak', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('stopsong', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('switchreset', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('switchselect', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('switchleftb', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('switchrightb', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('trackersupport', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('tsound', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('unlockzone', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('reboot', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('restorescreen', vscode.CompletionItemKind.Function),
                                    new vscode.CompletionItem('savescreen', vscode.CompletionItemKind.Function)
                                ];
            
                                // Append in source stuff
            
                                // Result
                                return items;
                             }
                        }
                    );
                    context.subscriptions.push(languageProvider);  */
            // If
            let ifProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `if `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('if ')) {
                        return undefined;
                    }
                    // TODO: add list of functions from base source
                    return [
                        new vscode.CompletionItem('boxcollision', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0up', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0down', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0left', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0right', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0any', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0fire', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0fire0', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0fire1', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0fire2', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0fire3', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0fire4', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0fire5', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0select', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy0start', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1up', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1down', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1left', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1right', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1any', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1fire', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1fire0', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1fire1', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1fire2', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1fire3', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1fire4', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1fire5', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1select', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('joy1start', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('mousex0', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('mousex1', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('mousey0', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('mousey1', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('drivingposition0', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('drivingposition1', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('paddleposition0', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('paddleposition1', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('paddleposition2', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('paddleposition3', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('pokeydetected', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('switchreset', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('switchselect', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('switchleftb', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('switchrightb', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0key0', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0key1', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0key2', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0key3', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0key4', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0key5', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0key6', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0key7', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0key8', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0key9', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0keys', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad0keyh', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1key0', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1key1', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1key2', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1key3', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1key4', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1key5', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1key6', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1key7', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1key8', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1key9', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1keys', vscode.CompletionItemKind.Function),
                        new vscode.CompletionItem('keypad1keyh', vscode.CompletionItemKind.Function)
                    ];
                }
            }, ' ');
            // return
            let returnProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('return ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('thisbank', vscode.CompletionItemKind.Keyword),
                        new vscode.CompletionItem('otherbank', vscode.CompletionItemKind.Keyword),
                        new vscode.CompletionItem('bank', vscode.CompletionItemKind.Keyword)
                    ];
                }
            }, ' ');
            // romsize
            let romsizeProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('romsize ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('16k', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('32K', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('48k', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('128k', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('128kRAM', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('128kBANKRAM', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('144k', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('256k', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('256kRAM', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('256kBANKRAM', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('272k', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('512k', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('512kRAM', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('512BANKRAM', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('528k', vscode.CompletionItemKind.Value)
                    ];
                }
            }, ' ');
            // displaymode
            let displayModeProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('displaymode ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('160A', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('160B', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('320A', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('320B', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('320C', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('320D', vscode.CompletionItemKind.Value)
                    ];
                }
            }, ' ');
            // shakescreen
            let shakescreenModeProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('shakescreen ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('lo', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('med', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('hi', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('off', vscode.CompletionItemKind.Value)
                    ];
                }
            }, ' ');
            // debug 
            let debugProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('debug ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('color', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('frames', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('interrupt', vscode.CompletionItemKind.Value)
                    ];
                }
            }, ' ');
            // deprecated 
            let deprecatedModeProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('deprecated ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('frameheight', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('160bindexes', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('boxcollision', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('onepass', vscode.CompletionItemKind.Value)
                    ];
                }
            }, ' ');
            // drawhiscores
            let drawhiscoresProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('drawhiscores ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('attract', vscode.CompletionItemKind.Keyword),
                        new vscode.CompletionItem('single', vscode.CompletionItemKind.Keyword),
                        new vscode.CompletionItem('player1', vscode.CompletionItemKind.Keyword),
                        new vscode.CompletionItem('player2', vscode.CompletionItemKind.Keyword)
                    ];
                }
            }, ' ');
            // tallsprite
            let tallspriteModeProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('tallsprite ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('off', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('on', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('spritesheet', vscode.CompletionItemKind.Value)
                    ];
                }
            }, ' ');
            // changedmaholes
            let changeDmaHolesProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('changedmaholes ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('disable', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('enable', vscode.CompletionItemKind.Value)
                    ];
                }
            }, ' ');
            // trackersupport
            let trackerSupportProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('trackersupport ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('basic', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('rmt', vscode.CompletionItemKind.Value)
                    ];
                }
            }, ' ');
            // optimization
            let optimizationModeProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('optimization ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('speed', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('size', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('noinlinedata', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('inlinerand', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('none', vscode.CompletionItemKind.Value)
                    ];
                }
            }, ' ');
            // autodim
            let autodimModeProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `return `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('autodim ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('init', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('byte', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('8.8', vscode.CompletionItemKind.Value),
                        new vscode.CompletionItem('4.4', vscode.CompletionItemKind.Value)
                    ];
                }
            }, ' ');
            // set
            let setProvider = vscode.languages.registerCompletionItemProvider(this.Id, {
                // return list of available language methods
                provideCompletionItems(document, position) {
                    // get all text until the `position` and check if it reads `set `
                    let linePrefix = document.lineAt(position).text.substring(0, position.character);
                    if (!linePrefix.endsWith('set ')) {
                        return undefined;
                    }
                    return [
                        new vscode.CompletionItem('romsize', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('doublewide', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('debug', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('tallsprite', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('tv', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('xm', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('zoneheight', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('frameskipfix', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('screenheight', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('extradlmemory', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('dlmemory', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('deprecated', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('dumpgraphics', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('collisionwrap', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('plotvalueonscreen', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('plotvaluepage', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('zoneprotection', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('pauseroutine', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('pausesilence', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('pokeysound', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('pokeysupport', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('pokeysfxsupport', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('tiasfx', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('trackersupport', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('fourbitfade', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('tiavolume', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('rmtvolume', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('rmtspeed', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('paddlerange', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('paddlepair', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('paddlescalex2', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('drivingboost', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('mousexonly', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('mousetime', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('tightpackborder', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('trakxonly', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('traktime', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('avoxvoice', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('mcpdevcart', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('multibutton', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('canary', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('crashdump', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('breakprotect', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('basepath', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('bankset', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('hssupport', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('hscsupport', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('hsgamename', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('hsseconds', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('hsscoresize', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('hscolorbase', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('hsdifficultytext', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('hsgameranks', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('snes0pause', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('snes1pause', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('snes#pause', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('multibuttonpause', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('7800GDmenuoff', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('softresetpause', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('smartbranching', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('7800header', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('optimization', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('backupstyle', vscode.CompletionItemKind.Method),
                        new vscode.CompletionItem('backupfile', vscode.CompletionItemKind.Method)
                    ];
                }
            }, ' ');
            // add items           
            context.subscriptions.push(ifProvider, returnProvider, romsizeProvider, displayModeProvider, shakescreenModeProvider, debugProvider, deprecatedModeProvider, drawhiscoresProvider, tallspriteModeProvider, changeDmaHolesProvider, trackerSupportProvider, optimizationModeProvider, autodimModeProvider, setProvider);
        });
    }
}
exports.SeventyEightHundredBasicCompletion = SeventyEightHundredBasicCompletion;
//# sourceMappingURL=seventyEightHundredBasicCompletion.js.map