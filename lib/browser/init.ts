import { EventEmitter } from 'events';
import * as fs from 'fs';
import * as path from 'path';

const Module = require('module');

// We modified the original process.argv to let node.js load the init.js,
// we need to restore it here.
process.argv.splice(1, 1);

// Clear search paths.
require('../common/reset-search-paths');

// Import common settings.
require('@electron/internal/common/init');

process._linkedBinding('electron_browser_event_emitter').setEventEmitterPrototype(EventEmitter.prototype);

// Don't quit on fatal error.
process.on('uncaughtException', function (error) {
  // Do nothing if the user has a custom uncaught exception handler.
  if (process.listenerCount('uncaughtException') > 1) {
    return;
  }

  // Show error in GUI.
  // We can't import { dialog } at the top of this file as this file is
  // responsible for setting up the require hook for the "electron" module
  // so we import it inside the handler down here
  import('electron')
    .then(({ dialog }) => {
      const stack = error.stack ? error.stack : `${error.name}: ${error.message}`;
      const message = 'Uncaught Exception:\n' + stack;
      dialog.showErrorBox('A JavaScript error occurred in the main process', message);
    });
});

// Emit 'exit' event on quit.
const { app } = require('electron');

app.on('quit', (_event, exitCode) => {
  process.emit('exit', exitCode);
});

if (process.platform === 'win32') {
  // If we are a Squirrel.Windows-installed app, set app user model ID
  // so that users don't have to do this.
  //
  // Squirrel packages are always of the form:
  //
  // PACKAGE-NAME
  // - Update.exe
  // - app-VERSION
  //   - OUREXE.exe
  //
  // Squirrel itself will always set the shortcut's App User Model ID to the
  // form `com.squirrel.PACKAGE-NAME.OUREXE`. We need to call
  // app.setAppUserModelId with a matching identifier so that renderer processes
  // will inherit this value.
  const updateDotExe = path.join(path.dirname(process.execPath), '..', 'update.exe');

  if (fs.existsSync(updateDotExe)) {
    const packageDir = path.dirname(path.resolve(updateDotExe));
    const packageName = path.basename(packageDir).replace(/\s/g, '');
    const exeName = path.basename(process.execPath).replace(/\.exe$/i, '').replace(/\s/g, '');

    app.setAppUserModelId(`com.squirrel.${packageName}.${exeName}`);
  }
}

// Map process.exit to app.exit, which quits gracefully.
process.exit = app.exit as () => never;

// Load the RPC server.
require('@electron/internal/browser/rpc-server');

// Load the guest view manager.
require('@electron/internal/browser/guest-view-manager');
require('@electron/internal/browser/guest-window-proxy');

// Now we try to load app's package.json.
let packagePath = null;
let packageJson = null;
const searchPaths = ['app', 'app.asar', 'default_app.asar'];

if (process.resourcesPath) {
  for (packagePath of searchPaths) {
    try {
      packagePath = path.join(process.resourcesPath, packagePath);
      packageJson = Module._load(path.join(packagePath, 'package.json'));
      break;
    } catch {
      continue;
    }
  }
}

if (packageJson == null) {
  process.nextTick(function () {
    return process.exit(1);
  });
  throw new Error('Unable to find a valid app');
}

// Set application's version.
if (packageJson.version != null) {
  app.setVersion(packageJson.version);
}

// Set application's name.
if (packageJson.productName != null) {
  app.name = `${packageJson.productName}`.trim();
} else if (packageJson.name != null) {
  app.name = `${packageJson.name}`.trim();
}

// Set application's desktop name.
if (packageJson.desktopName != null) {
  app.setDesktopName(packageJson.desktopName);
} else {
  app.setDesktopName(`${app.name}.desktop`);
}

// Set v8 flags, deliberately lazy load so that apps that do not use this
// feature do not pay the price
if (packageJson.v8Flags != null) {
  require('v8').setFlagsFromString(packageJson.v8Flags);
}

app._setDefaultAppPaths(packagePath);

// Load the chrome devtools support.
require('@electron/internal/browser/devtools');

if (BUILDFLAG(ENABLE_REMOTE_MODULE)) {
  require('@electron/internal/browser/remote/server');
}

// Load protocol module to ensure it is populated on app ready
require('@electron/internal/browser/api/protocol');

// Load web-contents module to ensure it is populated on app ready
require('@electron/internal/browser/api/web-contents');

// Load web-frame-main module to ensure it is populated on app ready
require('@electron/internal/browser/api/web-frame-main');

// Set main startup script of the app.
const mainStartupScript = packageJson.main || 'index.js';

const KNOWN_XDG_DESKTOP_VALUES = ['Pantheon', 'Unity:Unity7', 'pop:GNOME'];

function currentPlatformSupportsAppIndicator () {
  if (process.platform !== 'linux') return false;
  const currentDesktop = process.env.XDG_CURRENT_DESKTOP;

  if (!currentDesktop) return false;
  if (KNOWN_XDG_DESKTOP_VALUES.includes(currentDesktop)) return true;
  // ubuntu based or derived session (default ubuntu one, communitheme…) supports
  // indicator too.
  if (/ubuntu/ig.test(currentDesktop)) return true;

  return false;
}

// Workaround for electron/electron#5050 and electron/electron#9046
process.env.ORIGINAL_XDG_CURRENT_DESKTOP = process.env.XDG_CURRENT_DESKTOP;
if (currentPlatformSupportsAppIndicator()) {
  process.env.XDG_CURRENT_DESKTOP = 'Unity';
}

// Quit when all windows are closed and no other one is listening to this.
app.on('window-all-closed', () => {
  if (app.listenerCount('window-all-closed') === 1) {
    app.quit();
  }
});

const { setDefaultApplicationMenu } = require('@electron/internal/browser/default-menu');

// Create default menu.
//
// The |will-finish-launching| event is emitted before |ready| event, so default
// menu is set before any user window is created.
app.once('will-finish-launching', setDefaultApplicationMenu);

if (packagePath) {
  // Finally load app's main.js and transfer control to C++.
  process._firstFileName = Module._resolveFilename(path.join(packagePath, mainStartupScript), null, false);
  Module._load(path.join(packagePath, mainStartupScript), Module, true);
} else {
  console.error('Failed to locate a valid package to load (app, app.asar or default_app.asar)');
  console.error('This normally means you\'ve damaged the Electron package somehow');
}