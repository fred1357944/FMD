/**
 * Community plugin: fmd.community.vault-changelog
 * Appends one line to CHANGELOG.md on file-save. Off until community plugins
 * are enabled (Restricted Mode analog).
 */
function basename(path) {
  var p = String(path || "");
  var i = Math.max(p.lastIndexOf("/"), p.lastIndexOf("\\"));
  return i >= 0 ? p.slice(i + 1) : p;
}

function Plugin() {}

Plugin.prototype.onload = function (api) {
  api.workspace.on("file-save", function (payload) {
    var name = basename(payload && payload.path);
    if (!name || name === "CHANGELOG.md")
      return;
    var stamp = new Date().toISOString().slice(0, 19).replace("T", " ");
    var existing = api.vault.read("CHANGELOG.md");
    if (!existing)
      existing = "# Changelog\n\n";
    if (existing.length && existing.charAt(existing.length - 1) !== "\n")
      existing += "\n";
    api.vault.write("CHANGELOG.md", existing + "- " + stamp + "  " + name + "\n");
  });
};

Plugin.prototype.onunload = function () {};

module.exports = Plugin;
