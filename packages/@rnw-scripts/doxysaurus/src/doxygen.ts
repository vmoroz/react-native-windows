/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * This file contains some code taken from the doxygen project.
 * This is to fix/extend some doxygen package features.
 * https://github.com/EruantalonJS/node-doxygen
 * Copyright for the doxygen package code:
 * Copyright (c) 2016 Ivan Maria <iv.ivan.maria@gmail.com>;
 *
 * @format
 */

//
// Generates Doxygen documentation XML files.
//

// @ts-ignore (no typings for doxygen)
import constants = require('doxygen/lib/constants');
// @ts-ignore (no typings for doxygen)
import doxygen from 'doxygen';
import fs from 'fs';
import path from 'path';
import {exec} from 'child_process';
import {log} from './logger';
import {Config} from './config';

const DOXYGEN_VERSION = '1.9.1';

export async function generateDoxygenXml(config: Config) {
  const doxygenConfigPath = path.join(config.output, 'doxygen.config');
  generateDoxygenConfig(config, doxygenConfigPath);

  if (!isDoxygenExecutableInstalled(DOXYGEN_VERSION)) {
    log(`[Downloading] Doxygen version {${DOXYGEN_VERSION}} ...`);
    await doxygen.downloadVersion(DOXYGEN_VERSION);
    log(`[Downloaded] Doxygen version {${DOXYGEN_VERSION}}`);
  }

  const {stdout, stderr} = await runAsync(doxygenConfigPath, DOXYGEN_VERSION);

  for (const info of stdout.split('\n')) {
    log(`[Doxygen:] ${info}`);
  }

  // Doxygen process reports all warnings in the stderr stream.
  for (const warning of stderr.split('\n')) {
    log(`Warning: ${warning}`);
  }
}

function generateDoxygenConfig(config: Config, doxygenConfigPath: string) {
  const doxygenOptions: {[index: string]: string} = {
    OUTPUT_DIRECTORY: config.output,
    INPUT: config.input,
    GENERATE_LATEX: 'NO',
    GENERATE_HTML: 'NO',
    GENERATE_XML: 'YES',
  };

  if (config.filePatterns) {
    doxygenOptions.FILE_PATTERNS = config.filePatterns.join(' ');
  }

  doxygen.createConfig(doxygenOptions, doxygenConfigPath);
}

// Modified from doxygen NPM.
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

// Taken from doxygen NPM.
// It is required to use the fixed doxygenExecutablePath.
// The code is modified for TypeScript and eslint.
function isDoxygenExecutableInstalled(version?: any) {
  const execPath = doxygenExecutablePath(version);
  return fs.existsSync(execPath);
}

// Modified from doxygen NPM.
// Extends doxygen.run to return a Promise.
// We cannot use util.promisify because it loses the stderr with warnings.
async function runAsync(
  configPath?: any,
  version?: any,
): Promise<{stdout: string; stderr: string}> {
  configPath = configPath ? configPath : constants.path.configFile;
  const doxygenPath = doxygenExecutablePath(version);
  return new Promise((resolve, reject) => {
    exec(
      `"${doxygenPath}" "${path.resolve(configPath)}"`,
      (error, stdout, stderr) => {
        if (error) {
          reject(error);
        } else {
          resolve({stdout, stderr});
        }
      },
    );
  });
}
