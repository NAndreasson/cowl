/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global AddonManager, React */

"use strict";

loader.lazyRequireGetter(this, "React",
  "devtools/client/shared/vendor/react");
loader.lazyRequireGetter(this, "TargetList",
  "devtools/client/aboutdebugging/components/target-list", true);
loader.lazyRequireGetter(this, "TabHeader",
  "devtools/client/aboutdebugging/components/tab-header", true);
loader.lazyRequireGetter(this, "AddonsControls",
  "devtools/client/aboutdebugging/components/addons-controls", true);
loader.lazyRequireGetter(this, "Services");

loader.lazyImporter(this, "AddonManager",
  "resource://gre/modules/AddonManager.jsm");

const ExtensionIcon = "chrome://mozapps/skin/extensions/extensionGeneric.svg";
const Strings = Services.strings.createBundle(
  "chrome://devtools/locale/aboutdebugging.properties");

exports.AddonsTab = React.createClass({
  displayName: "AddonsTab",

  getInitialState() {
    return {
      extensions: [],
      debugDisabled: false,
    };
  },

  componentDidMount() {
    AddonManager.addAddonListener(this);
    Services.prefs.addObserver("devtools.chrome.enabled",
      this.updateDebugStatus, false);
    this.updateDebugStatus();
    this.updateAddonsList();
  },

  componentWillUnmount() {
    AddonManager.removeAddonListener(this);
    Services.prefs.removeObserver("devtools.chrome.enabled",
      this.updateDebugStatus);
  },

  render() {
    let { client } = this.props;
    let { debugDisabled, extensions: targets } = this.state;
    let name = Strings.GetStringFromName("extensions");

    return React.createElement(
      "div", { id: "tab-addons", className: "tab", role: "tabpanel",
        "aria-labelledby": "tab-addons-header-name" },
        React.createElement(TabHeader, {
          id: "tab-addons-header-name",
          name: Strings.GetStringFromName("addons")}),
        React.createElement(AddonsControls, { debugDisabled }),
        React.createElement(
          "div", { id: "addons" },
          React.createElement(TargetList,
            { name, targets, client, debugDisabled })
      )
    );
  },

  updateDebugStatus() {
    this.setState({
      debugDisabled: !Services.prefs.getBoolPref("devtools.chrome.enabled")
    });
  },

  updateAddonsList() {
    AddonManager.getAllAddons(addons => {
      let extensions = addons.filter(addon => addon.isDebuggable).map(addon => {
        return {
          name: addon.name,
          icon: addon.iconURL || ExtensionIcon,
          type: addon.type,
          addonID: addon.id
        };
      });
      this.setState({ extensions });
    });
  },

  /**
   * Mandatory callback as AddonManager listener.
   */
  onInstalled() {
    this.updateAddonsList();
  },

  /**
   * Mandatory callback as AddonManager listener.
   */
  onUninstalled() {
    this.updateAddonsList();
  },

  /**
   * Mandatory callback as AddonManager listener.
   */
  onEnabled() {
    this.updateAddonsList();
  },

  /**
   * Mandatory callback as AddonManager listener.
   */
  onDisabled() {
    this.updateAddonsList();
  },
});
