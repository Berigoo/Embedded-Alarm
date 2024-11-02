import { App, Plugin, PluginManifest, Editor, MarkdownView, PluginSettingTab } from 'obsidian';
import * as net from "net";
import "moment";
import * as crypto from "crypto"

//TODO make it less chaotic

// Define plugin settings
interface TaskAlarmSettings {
	serverHost: string;
	serverPort: number;
}

const DEFAULT_SETTINGS: TaskAlarmSettings = {
	serverHost: "192.168.1.4",
	serverPort: 8080,
};

export default class TaskAlarmPlugin extends Plugin {
	settings: TaskAlarmSettings;
	socket: net.Socket;

	async onload() {
		// Load settings
		await this.loadSettings();

		// Add command to create a task
		this.addCommand({
			id: "send_to_device",
			name: "Send to Device",
			callback: () => this.send(),
		});
		this.addCommand({
			id: "delete_from_device",
			name: "Delete from Device",
			callback: () => this.delete(),
		});
		this.addCommand({
			id: "fetch_from_device",
			name: "Fetch from Device",
			callback: () => this.fetch(),
		});
		this.addCommand({
			id: "next_mode",
			name: "Next Mode",
			callback: () => this.nextMode(),
		})

		// Add settings tab to configure the server URL
		this.addSettingTab(new TaskAlarmSettingTab(this.app, this));
	}

	onunload() {
		console.log("Task Alarm Plugin unloaded.");
	}

	async loadSettings() {
		this.settings = Object.assign({}, DEFAULT_SETTINGS, await this.loadData());
	}

	async saveSettings() {
		await this.saveData(this.settings);
	}

	async send(){
		this._sendTaskToServer().then(()=>{
			console.log("Success");
		}).catch(()=>{
			console.log("Failed");
		})
	}
	async delete(){
		this._deletetask().then(()=>{
			console.log("Success");
		}).catch(()=>{
			console.log("Failed");
		});
	}
	async fetch(){
		this._fetchTask().then(()=>{
			console.log("Success");
		}).catch(()=>{
			console.log("Failed");
		})
	}
	async nextMode(){
		this._nextMode().then(()=>{
			console.log("Success");
		}).catch(()=>{
			console.log("Failed");
		});
	}
	async _sendTaskToServer(){
		const activeView = this.app.workspace.getActiveViewOfType(MarkdownView);
		if(activeView){
			const currentLine = activeView.editor.getCursor().line;
			const text = activeView.editor.getLine(currentLine).trim();

			const textRegex = /^(?:- \[.\] *)?(.+?(?=\[|[\r\n]|$))/g
			const dueRegex = /(?:(?<=(?:\[due::.*))(?:(\d{4})-(\d{2})-(\d{2}))(?:[\s,T](\d{2}):(\d{2})(?::(\d{2}))?)?)/g;
			const scheduledRegex = /(?:(?<=(?:\[scheduled::.*))(?:(\d{4})-(\d{2})-(\d{2}))(?:[\s,T](\d{2}):(\d{2})(?::(\d{2}))?)?)/g

			const textMatch = textRegex.exec(text)
			const scheduledMatch = scheduledRegex.exec(text);
			const dueMatch = dueRegex.exec(text);
			const socket = new net.Socket();
			socket.setTimeout(10000);
			return new Promise<String>((resolve, reject)=>{
				const timeout = setTimeout(()=>{
					reject();
				}, 10000)
				socket.connect({host:this.settings.serverHost, port:this.settings.serverPort}, ()=>{
					socket.on("data", (data)=>{
						const ret = data.toString();
						if(ret == "ok"){
							resolve(ret);
						}else{
							reject(ret);
						}
						clearTimeout(timeout);
					})
					socket.on("close", ()=>{
						reject();
						clearTimeout(timeout);
					})
					socket.on("end", ()=>{
						socket.end();
						resolve("ok");
						clearTimeout(timeout);
					})

					const command = 0x01;
					let message = "no name";
					if(textMatch != null)
						message = textMatch[1].trim();
					const sha256 = crypto.createHash("sha256").update(message);

					const messageBuffer = Buffer.from(message, "utf-8");
					const messageLen = messageBuffer.length
					const id = sha256.copy().digest('hex').slice(0, 32);
					const idBuffer = Buffer.from(id, 'hex');

					//TODO
					let due = BigInt(0);
					if(dueMatch != null){
						if(dueMatch[4] != undefined){
							console.log("time exist");
							due = BigInt(Date.parse(dueMatch.slice(1, 4).join('-') + 'T' + dueMatch.slice(4, 6).join(':')))
						}else{
							due = BigInt(Date.parse(dueMatch?.slice(1, 4).join('-')));
						}
					}
					let scheduled = BigInt(0);
					if(scheduledMatch != null){
						if(scheduledMatch[4] != undefined){
							scheduled = BigInt(Date.parse(scheduledMatch.slice(1, 4).join('-') + 'T' + scheduledMatch.slice(4, 6).join(':')))
						}else{
							scheduled = BigInt(Date.parse(scheduledMatch?.slice(1, 4).join('-')));
						}
					}

					console.log(due);

					const buf = new Buffer(1 + 4 + 32 + 16 + messageLen)

					const timeBuffer = Buffer.alloc(16);
					timeBuffer.writeBigUInt64LE(due, 0);
					timeBuffer.writeBigUInt64LE(scheduled, 8);

					buf.writeUint8(command);
					buf.writeUint32LE(messageLen, 1);
					idBuffer.copy(buf, 5);
					timeBuffer.copy(buf, 5+32);
					messageBuffer.copy(buf, 5+32+16);
					socket.write(buf);
				});
			});
		}
	}

	async _deletetask(){
		const activeView = this.app.workspace.getActiveViewOfType(MarkdownView);
		if(activeView){
			const currentLine = activeView.editor.getCursor().line;
			const text = activeView.editor.getLine(currentLine).trim();

			const textRegex = /^(?:- \[.\] *)?(.+?(?=\[|[\r\n]|$))/g
			const textMatch = textRegex.exec(text);

			const socket = new net.Socket();
			socket.setTimeout(10000);
			return new Promise<String>((resolve, reject)=>{
				const timeout = setTimeout(()=>{
					reject();
				}, 10000)
				socket.connect({host:this.settings.serverHost, port:this.settings.serverPort}, ()=>{
					socket.on("data", (data)=>{
						const ret = data.toString();
						if(ret == "ok"){
							resolve(ret);
						}else{
							reject(ret);
						}
						clearTimeout(timeout);
					})
					socket.on("close", ()=>{
						reject();
						clearTimeout(timeout);
					})
					socket.on("end", ()=>{
						socket.end();
						resolve("ok");
						clearTimeout(timeout);
					})

					if(textMatch != null){
						const sha256 = crypto.createHash("sha256").update(textMatch[1].trim());
						const id = sha256.copy().digest('hex').slice(0, 32);
						const buf = new Buffer(1 + 32);
						buf.writeUint8(0x02);
						const idBuffer = Buffer.from(id, 'hex');
						idBuffer.copy(buf, 1);
						socket.write(buf);
					}else{
						reject()
						clearTimeout(timeout);
					}
				});
			});
		}

	}

	async _fetchTask(){
		const socket = new net.Socket();
		socket.setTimeout(10000);
		let isRet = false;
		return new Promise<String>((resolve, reject)=>{
			const activeView = this.app.workspace.getActiveViewOfType(MarkdownView);
			if(!activeView) reject("not a markdown");
			const editor = activeView?.editor;
			const currentLine = activeView.editor.getCursor().line;
			let cursor: EditorPosition = {
				line: currentLine,
				ch: 0
			}
			const timeout = setTimeout(()=>{
				reject();
			}, 10000)

			let buffer = Buffer.alloc(0);
			let expectedSize = 100000;
			socket.connect({host:this.settings.serverHost, port:this.settings.serverPort}, ()=>{
				socket.on("data", (data)=>{
					//TODO make data accumulation less bad
					if(buffer.length < expectedSize){
						buffer = Buffer.concat([buffer, data]);
						expectedSize = buffer.readUint32LE(0);
					}
					if(buffer.length === expectedSize){
						const text = buffer.slice(4, 4 + (buffer.length - 16 - 4)).toString();
						const dueNum = Number(buffer.readBigUint64LE(buffer.length - 16))*1000;
						const scheduledNum = Number(buffer.readBigUint64LE(buffer.length - 8))*1000; 
						const due = new Date(dueNum);
						const scheduled = new Date(scheduledNum);
						
						console.log(new Date(Number(buffer.readBigUint64LE(buffer.length - 16))*1000));
						const dueDate = moment(due).format("yyyy-MM-DDTHH:mm:ss");
						const scheduledDate = moment(scheduled).format("yyyy-MM-DDTHH:mm:ss");
						let string = `- [ ] ${text}`;
						if(dueNum != 0) string += ` [due:: ${dueDate}]`;
						if(scheduledNum != 0) string += ` [scheduled:: ${scheduledDate}]`;
						string += "\n";

						editor.replaceRange(string, cursor);
						socket.write("ok");
						buffer = Buffer.alloc(0);
						expectedSize = 100000;
					}
				})
				socket.on("close", ()=>{
					reject();
					clearTimeout(timeout);
				})
				socket.on("end", ()=>{
					socket.end();
					resolve("ok");
					clearTimeout(timeout);
				})

				const buf = new Buffer(1);
				buf.writeUint8(0x08);
				socket.write(buf);
			});
		});
	}
	async _nextMode(){
		const socket = new net.Socket();
		socket.setTimeout(10000);
		return new Promise<String>((resolve, reject)=>{
			const timeout = setTimeout(()=>{
				reject();
			}, 10000)

			socket.connect({host:this.settings.serverHost, port:this.settings.serverPort}, ()=>{
				socket.on("close", ()=>{
					reject();
					clearTimeout(timeout);
				})
				socket.on("end", ()=>{
					socket.end();
					resolve("ok");
					clearTimeout(timeout);
				})

				const buf = new Buffer(1);
				buf.writeUint8(0x10);
				socket.write(buf);
			});
		});
	}
}

class TaskAlarmSettingTab extends PluginSettingTab {
	plugin: TaskAlarmPlugin;

	constructor(app: App, plugin: TaskAlarmPlugin) {
		super(app, plugin);
		this.plugin = plugin;
	}

	display(): void {
		const { containerEl } = this;

		containerEl.empty();

		containerEl.createEl("h2", { text: "Task Alarm Plugin Settings" });

		new Setting(containerEl)
		.setName("Server Host")
		.setDesc("The adress of your host server")
		.addText((text)=>{
			text
			.setValue(this.plugin.settings.serverHost)
			.onChange(async (val)=>{
				this.plugin.settings.serverHost = val;
				await this.plugin.saveSettings();
			})

		})
		new Setting(containerEl)
		.setName("Server Port")
		.setDesc("The port of your host server")
		.addText((text)=>{
			text
			.setValue(this.plugin.settings.serverPort.toString())
			.onChange(async (val)=>{
				this.plugin.settings.serverPort = parseInt(val);
				await this.plugin.saveSettings();
			})

		})

	}
}
