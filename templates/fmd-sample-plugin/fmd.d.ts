/**
 * FMD host plugin API typings (TSDoc for humans + AI autocomplete).
 * Ship alongside the sample plugin; later publish as `@fmd/types`.
 *
 * Runtime global: `fmd` (alias `api`) injected by PluginHost (QJSEngine).
 * Extend the {@link Plugin} class from `./fmd` (fmd.ts runtime shim).
 */

/** Declared permissions in manifest.json */
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
  /** Stable id — do not rename after release */
  id: string;
  name: string;
  shortcut?: string;
  callback: () => void;
}

/** Host API injected as `fmd` / `api` */
export interface FmdApi {
  pluginId: string;
  workspace: {
    getActiveFile(): ActiveFile | Record<string, never> | null;
    on(event: WorkspaceEvent, cb: (payload: { path?: string; [k: string]: unknown }) => void): void;
    off(event: WorkspaceEvent, cb: (...args: unknown[]) => void): void;
  };
  editor: {
    /** Requires `editor.read` */
    getValue(): string;
    getSelection(): string;
    /** Requires `editor.write` */
    setValue(text: string): void;
    replaceSelection(text: string): void;
  };
  ui: {
    /** Requires `ui.statusbar` */
    setStatusBarItem(item: StatusBarItem): void;
    notice(message: string): void;
  };
  commands: {
    /** Requires `commands.register` */
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

declare global {
  /** Injected by FMD PluginHost */
  const fmd: FmdApi;
  const api: FmdApi;
}

export {};
