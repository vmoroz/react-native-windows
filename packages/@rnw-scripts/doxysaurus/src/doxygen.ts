/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * This file contains some code taken from the doxygen NPM package.
 * This is to fix/extend some doxygen package features.
 * https://github.com/EruantalonJS/node-doxygen
 * Copyright for the doxygen package code:
 * Copyright (c) 2016 Ivan Maria <iv.ivan.maria@gmail.com>;
 *
 * @format
 */

//
// Generate Doxygen documentation XML files.
//

// @ts-ignore (no typings for doxygen)
import * as doxygen from 'doxygen';
import * as fs from 'fs';
import * as path from 'path';
import {spawn} from 'child_process';
import {Config} from './config';
import {log} from './logger';

const DOXYGEN_VERSION = '1.9.1';

export async function generateDoxygenXml(config: Config) {
  const doxygenConfigPath = path.join(config.output, 'doxygen.config');
  generateDoxygenConfig(config, doxygenConfigPath);

  if (!isDoxygenExecutableInstalled(DOXYGEN_VERSION)) {
    log(`[Downloading] Doxygen version {${DOXYGEN_VERSION}} ...`);
    await doxygen.downloadVersion(DOXYGEN_VERSION);
    log(`[Downloaded] Doxygen version {${DOXYGEN_VERSION}}`);
  }

  await run(doxygenConfigPath, DOXYGEN_VERSION);
}

function generateDoxygenConfig(config: Config, doxygenConfigPath: string) {
  const doxygenOptions = {
    OUTPUT_DIRECTORY: config.output,
    INPUT: config.input,
    GENERATE_LATEX: 'NO',
    GENERATE_HTML: 'NO',
    GENERATE_XML: 'YES',
  };

  doxygen.createConfig(doxygenOptions, doxygenConfigPath);
}

// From doxygen NPM.
// To access platform-specific constants
// @ts-ignore (no typings for doxygen)
import constants = require('doxygen/lib/constants');

// From doxygen NPM.
// the goal is to use the unpublished fix for Windows path:
// https://github.com/EruantalonJS/node-doxygen/pull/34
// The code is modified for TypeScript, eslint, and the different __dirname.
function doxygenExecutablePath(version?: any) {
  version = version ? version : constants.default.version;
  // const dirName = __dirname; -- we must use the doxygen package path
  const dirName = path.dirname(require.resolve('doxygen/package.json'));

  const doxygenFolder =
    process.platform === constants.platform.macOS.identifier
      ? constants.path.macOsDoxygenFolder
      : '';

  const ext =
    process.platform === constants.platform.windows.identifier ? '.exe' : '';
  return path.normalize(
    path.join(dirName, 'dist', version, doxygenFolder, 'doxygen' + ext),
  );
}

// From doxygen NPM.
// It is required to use the fixed doxygenExecutablePath.
// The code is modified for TypeScript and eslint.
function isDoxygenExecutableInstalled(version?: any) {
  const execPath = doxygenExecutablePath(version);
  return fs.existsSync(execPath);
}

// From doxygen NPM.
// Extends doxygen.run to use our logger and to return Promise.
async function run(configPath?: any, version?: any) {
  configPath = configPath ? configPath : constants.path.configFile;
  const doxygenPath = doxygenExecutablePath(version);

  // Show all warnings/errors in the end to avoid mixing them with notifications.
  let errors: string[] = [];
  const doxygenProcess = spawn(doxygenPath, [path.resolve(<string>configPath)]);

  doxygenProcess.stdout.on('data', data => {
    const lines = (data + '').split('\n');
    for (const line of lines) {
      log(`[Doxygen:] ${line}`);
    }
  });

  doxygenProcess.stderr.on('data', data => {
    errors = errors.concat((data + '').split('\n'));
  });

  return new Promise((resolve, reject) => {
    doxygenProcess.on('close', (code, signal) => {
      if (code === 0) {
        for (const warning of errors) {
          log(`Warning: ${warning}`);
        }

        resolve(null);
      } else {
        for (const error of errors) {
          log(`Error: ${error}`);
        }

        console.log(`Error: {${doxygenPath}} exited with code {${code}}`);
        reject(new Error(signal + ''));
      }
    });
  });
}
