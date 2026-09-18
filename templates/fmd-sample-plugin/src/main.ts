import { Plugin, type FmdApi } from "../fmd";
import { DEFAULT_SETTINGS, type SamplePluginSettings } from "./settings";

/**
 * Sample FMD plugin — mirrors Obsidian sample-plugin teaching style.
 * Bundled by esbuild to root `main.js` (CommonJS).
 */
class SamplePlugin extends Plugin {
  settings: SamplePluginSettings = { ...DEFAULT_SETTINGS };

  async onload(api?: FmdApi) {
    if (api) this.api = api;
    this.loadSettings();

    this.addCommand({
      id: "sample-hello",
      name: "Say hello",
      callback: () => {
        this.api.ui.notice("Hello from the FMD sample plugin!");
      },
    });

    this.addCommand({
      id: "sample-wordcount",
      name: "Refresh status word count",
      callback: () => this.updateStatus(),
    });

    this.api.workspace.on("file-open", () => this.updateStatus());
    this.api.workspace.on("file-change", () => this.updateStatus());
    this.updateStatus();
  }

  onunload() {
    // Engine teardown clears listeners; keep idempotent cleanup here if needed.
  }

  private loadSettings() {
    const enabled = this.api.settings.get("enabled", DEFAULT_SETTINGS.enabled);
    const statusPrefix = this.api.settings.get(
      "statusPrefix",
      DEFAULT_SETTINGS.statusPrefix,
    );
    this.settings = {
      enabled: Boolean(enabled),
      statusPrefix: String(statusPrefix),
    };
  }

  private saveSettings() {
    this.api.settings.set("enabled", this.settings.enabled);
    this.api.settings.set("statusPrefix", this.settings.statusPrefix);
  }

  private updateStatus() {
    if (!this.settings.enabled) return;
    const text = this.api.editor.getValue() || "";
    const words = text.trim() ? text.trim().split(/\s+/).filter(Boolean).length : 0;
    this.api.ui.setStatusBarItem({
      id: "sample-plugin-status",
      text: `${this.settings.statusPrefix}: ${words} words`,
    });
    this.saveSettings();
  }
}

// Host expects CommonJS constructor export (esbuild format: cjs)
module.exports = SamplePlugin;
