/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import * as fsUtils from './fs-utils';
import * as path from 'path';
import * as fs from 'fs';
import {ensureAbsolutePath} from './fs-utils';
const fsPromises = fs.promises;

// Definition of a config.
export interface Config {
  configDir: string;
  input: string;
  output: string;
  prefix: string;
  index: string;
  projects?: string[];
}

// Generates an async stream of project configs.
export async function* getProjectConfigs(configPath: string) {
  const config = await loadConfig(
    ensureAbsolutePath(configPath, process.cwd()),
  );
  if (config.projects) {
    for (const project of config.projects) {
      const projectConfigPath = ensureAbsolutePath(
        path.join(project, 'doxysaurus.json'),
        config.configDir,
      );
      if (fs.existsSync(projectConfigPath)) {
        yield loadConfig(projectConfigPath, config);
      } else {
        yield updateConfig(config, {input: project});
      }
    }
  } else {
    yield normalizeConfig(config);
  }
}

// Loads config file and merges it with parent config when it is given.
export async function loadConfig(
  configPath: string,
  parentConfig?: Config,
): Promise<Config> {
  const configText = await fsPromises.readFile(configPath, 'utf-8');
  const config = <Partial<Config>>JSON.parse(configText);
  config.configDir = path.dirname(configPath);
  if (parentConfig) {
    return updateConfig(
      parentConfig,
      {input: undefined, projects: undefined},
      config,
    );
  } else {
    return normalizeConfig(config);
  }
}

// Normalizes config to set all required fields and make all paths absolute.
function normalizeConfig(config: Partial<Config>): Config {
  if (config.configDir) {
    if (config.input) {
      config.input = fsUtils.ensureAbsolutePath(config.input, config.configDir);
    } else {
      config.input = config.configDir;
    }

    if (config.output) {
      if (!path.isAbsolute(config.output)) {
        config.output = path.join(config.configDir, config.output);
      }
    } else {
      config.output = path.join(process.cwd(), 'docs');
    }
    config.output = path.normalize(config.output);

    if (!config.prefix) {
      config.prefix = '';
    }

    if (!config.index) {
      config.index = 'index.md';
    }

    if (!config.projects) {
      delete config.projects;
    }

    return <Config>config;
  } else {
    throw new Error('Undefined config.configDir');
  }
}

// Updates and normalizes config file.
function updateConfig(config: Config, ...update: Partial<Config>[]): Config {
  return normalizeConfig(Object.assign({}, config, ...update));
}
