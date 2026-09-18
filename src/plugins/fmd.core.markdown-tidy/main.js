/**
 * Builtin plugin: fmd.core.markdown-tidy
 * Proves editor.read/write + commands.register (statusbar-stats is read-only).
 */
function tidyMarkdown(source) {
  var text = String(source || "").replace(/\r\n/g, "\n").replace(/\r/g, "\n");
  var lines = text.split("\n");
  var out = [];
  var inFence = false;
  var blankRun = 0;
  for (var i = 0; i < lines.length; i++) {
    var line = lines[i].replace(/[ \t]+$/, "");
    var trimmed = line.trim();
    if (trimmed.indexOf("```") === 0) {
      inFence = !inFence;
      blankRun = 0;
      out.push(line);
      continue;
    }
    if (line === "") {
      if (inFence) {
        out.push("");
        continue;
      }
      blankRun += 1;
      if (blankRun <= 1)
        out.push("");
      continue;
    }
    blankRun = 0;
    out.push(line);
  }
  while (out.length && out[out.length - 1] === "")
    out.pop();
  return out.join("\n") + "\n";
}

function Plugin() {}

Plugin.prototype.onload = function (api) {
  var self = this;
  this.api = api;
  api.commands.register({
    id: "tidy",
    name: "Tidy Markdown",
    callback: function () {
      self.tidy();
    },
  });
};

Plugin.prototype.tidy = function () {
  var api = this.api;
  var text = api.editor.getValue() || "";
  var cleaned = tidyMarkdown(text);
  if (cleaned === text) {
    api.ui.notice("Already tidy");
    return;
  }
  api.editor.setValue(cleaned);
  api.ui.notice("Tidied Markdown");
};

Plugin.prototype.onunload = function () {};

module.exports = Plugin;
