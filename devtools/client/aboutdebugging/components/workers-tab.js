/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global React */

"use strict";

loader.lazyRequireGetter(this, "Ci",
  "chrome", true);
loader.lazyRequireGetter(this, "React",
  "devtools/client/shared/vendor/react");
loader.lazyRequireGetter(this, "TargetList",
  "devtools/client/aboutdebugging/components/target-list", true);
loader.lazyRequireGetter(this, "TabHeader",
  "devtools/client/aboutdebugging/components/tab-header", true);
loader.lazyRequireGetter(this, "Services");

loader.lazyImporter(this, "Task", "resource://gre/modules/Task.jsm");

const Strings = Services.strings.createBundle(
  "chrome://devtools/locale/aboutdebugging.properties");
const WorkerIcon = "chrome://devtools/skin/images/debugging-workers.svg";

exports.WorkersTab = React.createClass({
  displayName: "WorkersTab",

  getInitialState() {
    return {
      workers: {
        service: [],
        shared: [],
        other: []
      }
    };
  },

  componentDidMount() {
    let client = this.props.client;
    client.addListener("workerListChanged", this.update);
    client.addListener("processListChanged", this.update);
    this.update();
  },

  componentWillUnmount() {
    let client = this.props.client;
    client.removeListener("processListChanged", this.update);
    client.removeListener("workerListChanged", this.update);
  },

  render() {
    let { client } = this.props;
    let { workers } = this.state;

    return React.createElement(
      "div", { id: "tab-workers", className: "tab", role: "tabpanel",
        "aria-labelledby": "tab-workers-header-name" },
        React.createElement(TabHeader, {
          id: "tab-workers-header-name",
          name: Strings.GetStringFromName("workers")}),
        React.createElement(
          "div", { id: "workers", className: "inverted-icons" },
          React.createElement(TargetList, {
            id: "service-workers",
            name: Strings.GetStringFromName("serviceWorkers"),
            targets: workers.service, client }),
          React.createElement(TargetList, {
            id: "shared-workers",
            name: Strings.GetStringFromName("sharedWorkers"),
            targets: workers.shared, client }),
          React.createElement(TargetList, {
            id: "other-workers",
            name: Strings.GetStringFromName("otherWorkers"),
            targets: workers.other, client }))
      );
  },

  update() {
    let workers = this.getInitialState().workers;
    this.getWorkerForms().then(forms => {
      forms.forEach(form => {
        let worker = {
          name: form.url,
          icon: WorkerIcon,
          actorID: form.actor
        };
        switch (form.type) {
          case Ci.nsIWorkerDebugger.TYPE_SERVICE:
            worker.type = "serviceworker";
            workers.service.push(worker);
            break;
          case Ci.nsIWorkerDebugger.TYPE_SHARED:
            worker.type = "sharedworker";
            workers.shared.push(worker);
            break;
          default:
            worker.type = "worker";
            workers.other.push(worker);
        }
      });
      this.setState({ workers });
    });
  },

  getWorkerForms: Task.async(function*() {
    let client = this.props.client;

    // List workers from the Parent process
    let result = yield client.mainRoot.listWorkers();
    let forms = result.workers;

    // And then from the Child processes
    let { processes } = yield client.mainRoot.listProcesses();
    for (let process of processes) {
      // Ignore parent process
      if (process.parent) {
        continue;
      }
      let { form } = yield client.getProcess(process.id);
      let processActor = form.actor;
      let { workers } = yield client.request({to: processActor,
                                              type: "listWorkers"});
      forms = forms.concat(workers);
    }

    return forms;
  }),
});
