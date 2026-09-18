/**
 * Builtin plugin: fmd.core.statusbar-stats
 * Proves editor.read + workspace events + ui.statusbar seams.
 */
function Plugin() {}

Plugin.prototype.onload = function (api) {
  var self = this;
  this.api = api;

  function update() {
    var text = api.editor.getValue() || "";
    var trimmed = text.trim();
    var words = 0;
    if (trimmed.length) {
      // Simple whitespace split; CJK handled loosely
      words = trimmed.split(/\s+/).filter(Boolean).length;
    }
    var chars = text.length;
    api.ui.setStatusBarItem({
      id: "statusbar-stats",
      text: words + " words · " + chars + " chars (plugin)"
    });
  }

  api.workspace.on("file-open", update);
  api.workspace.on("file-change", update);
  api.workspace.on("file-save", update);
  update();

  this._update = update;
};

Plugin.prototype.onunload = function () {
  // Listeners cleared when engine is destroyed
};

module.exports = Plugin;
