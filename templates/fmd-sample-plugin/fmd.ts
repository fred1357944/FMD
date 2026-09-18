/**
 * Runtime shim for author DX — bundled into main.js.
 * Types live in fmd.d.ts; host still injects global `fmd`.
 */

export type FmdPermission =
  | "editor.read"
  | "editor.write"
  | "ui.statusbar"
  | "commands.register"
  | "frontmatter.read"
  | "frontmatter.write"
  | "vault.read"
  | "vault.write";

export interface FmdManifest {
  id: string;
  name: string;
  version: string;
  minAppVersion: string;
  description: string;
  author?: string;
  authorUrl?: string;
  fundingUrl?: string | Record<string, string>;
  main?: string;
  permissions?: FmdPermission[];
}

export interface ActiveFile {
  path: string;
  content: string;
  frontmatter?: Record<string, unknown>;
}

export type WorkspaceEvent =
  | "file-open"
  | "file-save"
  | "file-change"
  | "frontmatter-change"
  | "view-change";

export interface StatusBarItem {
  id: string;
  text: string;
}

export interface CommandDescriptor {
  id: string;
  name: string;
  shortcut?: string;
  callback: () => void;
}

export interface FmdApi {
  pluginId: string;
  workspace: {
    getActiveFile(): ActiveFile | Record<string, never> | null;
    on(event: WorkspaceEvent, cb: (payload: { path?: string; [k: string]: unknown }) => void): void;
    off(event: WorkspaceEvent, cb: (...args: unknown[]) => void): void;
  };
  editor: {
    getValue(): string;
    getSelection(): string;
    setValue(text: string): void;
    replaceSelection(text: string): void;
  };
  ui: {
    setStatusBarItem(item: StatusBarItem): void;
    notice(message: string): void;
  };
  commands: {
    register(cmd: CommandDescriptor): void;
  };
  settings: {
    get(key: string, defaultValue?: unknown): unknown;
    set(key: string, value: unknown): void;
  };
  frontmatter: {
    get(path?: string): Record<string, unknown>;
    set(key: string, value: unknown): void;
    update(patch: Record<string, unknown>): void;
  };
  vault: {
    read(path: string): string;
    write(path: string, text: string): void;
    list(folder: string): string[];
  };
}

/** Base class — host constructs your subclass and calls onload(api). */
export abstract class Plugin {
  api!: FmdApi;

  constructor(api?: FmdApi) {
    if (api) this.api = api;
  }

  abstract onload(api?: FmdApi): void | Promise<void>;

  onunload(): void {}

  addCommand(cmd: CommandDescriptor): void {
    this.api.commands.register(cmd);
  }
}
