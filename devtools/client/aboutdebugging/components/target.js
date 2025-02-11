/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global alert, BrowserToolboxProcess, gDevTools, React, TargetFactory,
   Toolbox */

"use strict";

loader.lazyRequireGetter(this, "React",
  "devtools/client/shared/vendor/react");
loader.lazyRequireGetter(this, "TargetFactory",
  "devtools/client/framework/target", true);
loader.lazyRequireGetter(this, "Toolbox",
  "devtools/client/framework/toolbox", true);
loader.lazyRequireGetter(this, "Services");

loader.lazyImporter(this, "BrowserToolboxProcess",
  "resource://devtools/client/framework/ToolboxProcess.jsm");
loader.lazyRequireGetter(this, "gDevTools",
  "devtools/client/framework/devtools", true);

const Strings = Services.strings.createBundle(
  "chrome://devtools/locale/aboutdebugging.properties");

exports.Target = React.createClass({
  displayName: "Target",

  debug() {
    let { client, target } = this.props;
    switch (target.type) {
      case "extension":
        BrowserToolboxProcess.init({ addonID: target.addonID });
        break;
      case "serviceworker":
        // Fall through.
      case "sharedworker":
        // Fall through.
      case "worker":
        let workerActor = this.props.target.actorID;
        client.attachWorker(workerActor, (response, workerClient) => {
          gDevTools.showToolbox(TargetFactory.forWorker(workerClient),
            "jsdebugger", Toolbox.HostType.WINDOW)
            .then(toolbox => {
              toolbox.once("destroy", () => workerClient.detach());
            });
        });
        break;
      default:
        alert("Not implemented yet!");
    }
  },

  render() {
    let { target, debugDisabled } = this.props;
    return React.createElement("div", { className: "target" },
      React.createElement("img", {
        className: "target-icon",
        role: "presentation",
        src: target.icon }),
      React.createElement("div", { className: "target-details" },
        React.createElement("div", { className: "target-name" }, target.name),
        React.createElement("div", { className: "target-url" }, target.url)
      ),
      React.createElement("button", {
        className: "debug-button",
        onClick: this.debug,
        disabled: debugDisabled,
      }, Strings.GetStringFromName("debug"))
    );
  },
});
