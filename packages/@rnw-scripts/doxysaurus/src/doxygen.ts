/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

// @ts-ignore (no typings for doxygen)
import * as doxygen from 'doxygen';
import {Config} from './config';
import * as path from 'path';

const DOXYGEN_VERSION = '1.9.1';

// Generates doxygen XMLs for the config.
export async function generateDoxygenXml(config: Config) {
  console.log(config);

  const doxygenConfigPath = path.join(config.output, 'doxygen.config');
  generateDoxygenConfig(config, doxygenConfigPath);

  if (!doxygen.isDoxygenExecutableInstalled(DOXYGEN_VERSION)) {
    console.log(`Downloading Doxygen version ${DOXYGEN_VERSION} ...`);
    await doxygen.downloadVersion(DOXYGEN_VERSION);
    console.log(`Doxygen version ${DOXYGEN_VERSION} is downloaded`);
  }

  doxygen.run(doxygenConfigPath, DOXYGEN_VERSION);
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
