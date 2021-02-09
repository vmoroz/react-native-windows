/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

/**
 * Configuration for the Doxysaurus tool.
 */

import * as path from 'path';
import * as t from 'io-ts';
import {promises as fs, existsSync} from 'fs';
import {PathReporter} from 'io-ts/lib/PathReporter';
import {log} from './logger';

/**
 * Schema for the "doxysaurus.json" config file.
 */
const ConfigSchemaType = t.type({
  configDir: t.union([t.undefined, t.string]),
  input: t.union([t.undefined, t.string]),
  buildDir: t.union([t.undefined, t.string]),
  outputDir: t.union([t.undefined, t.string]),
  filePatterns: t.union([t.undefined, t.array(t.string)]),
  docIdPrefix: t.union([t.undefined, t.string]),
  indexFilename: t.union([t.undefined, t.string]),
  indexTemplatePath: t.union([t.undefined, t.string]),
  projects: t.union([t.undefined, t.array(t.string)]),
  types: t.union([t.undefined, t.array(t.string)]),
  namespaceAliases: t.union([
    t.undefined,
    t.record(t.string, t.array(t.string)),
  ]),
  sections: t.union([t.undefined, t.record(t.string, t.string)]),
  stdTypeLinks: t.union([
    t.undefined,
    t.type({
      linkPrefix: t.union([t.undefined, t.string]),
      linkMap: t.record(t.string, t.string),
      operatorMap: t.union([t.undefined, t.record(t.string, t.string)]),
    }),
  ]),
  idlTypeLinks: t.union([
    t.undefined,
    t.type({
      linkPrefix: t.union([t.undefined, t.string]),
      linkMap: t.record(t.string, t.string),
      operatorMap: t.union([t.undefined, t.record(t.string, t.string)]),
    }),
  ]),
});

const LoadedConfigType = t.intersection([
  ConfigSchemaType,
  t.type({configDir: t.string, buildDir: t.string}),
]);

const ConfigType = t.intersection([
  LoadedConfigType,
  t.type({
    input: t.string,
    outputDir: t.string,
    docIdPrefix: t.string,
  }),
]);

/** Doxysaurus project configuration. */
export type ConfigSchema = t.TypeOf<typeof ConfigSchemaType>;
export type Config = t.TypeOf<typeof ConfigType>;
export type LoadedConfig = t.TypeOf<typeof LoadedConfigType>;

/** Generates an async stream of project configs. */
export async function* getProjectConfigs(
  configPath: string,
  outputDir?: string,
  parentConfig?: LoadedConfig,
): AsyncGenerator<Config> {
  const config = await loadConfig(
    ensureAbsolutePath(configPath, process.cwd()),
    parentConfig,
  );
  config.outputDir = outputDir;
  if (Array.isArray(config.projects) && config.projects.length > 0) {
    for (const project of config.projects) {
      const projectConfigPath = ensureAbsolutePath(
        path.join(project, 'doxysaurus.json'),
        config.configDir,
      );
      if (existsSync(projectConfigPath)) {
        for await (const projectConfig of getProjectConfigs(
          projectConfigPath,
          outputDir,
          config,
        )) {
          yield projectConfig;
        }
      } else {
        yield normalizeConfig({...config, input: project});
      }
    }
  } else {
    yield normalizeConfig(config);
  }
}

/** Loads config file and merges it with parent config if it is given. */
async function loadConfig(
  configPath: string,
  parentConfig?: LoadedConfig,
): Promise<LoadedConfig> {
  const configText = await fs.readFile(configPath, 'utf-8');
  const json = JSON.parse(configText);
  const decodeResult = ConfigSchemaType.decode(json);
  if (decodeResult._tag === 'Left') {
    const errors = PathReporter.report(decodeResult);
    log.error(`[Parse] config {${configPath}}`);
    for (const error of errors) {
      log.error(error);
    }
    process.exit(1);
  }
  const config = json as ConfigSchema;
  const configDir = path.dirname(configPath);
  if (parentConfig) {
    return {
      ...parentConfig,
      input: undefined,
      projects: undefined,
      ...config,
      configDir,
      buildDir: path.join(
        parentConfig.buildDir,
        config.buildDir || path.basename(configDir),
      ),
    };
  } else {
    return {
      ...config,
      configDir,
      buildDir: config.buildDir
        ? ensureAbsolutePath(config.buildDir, configDir)
        : path.join(process.cwd(), 'docs'),
    };
  }
}

/** Normalizes config to set all required fields and makes all paths absolute. */
function normalizeConfig(config: LoadedConfig): Config {
  const buildDir = config.buildDir
    ? ensureAbsolutePath(config.buildDir, config.configDir)
    : path.join(process.cwd(), 'docs');
  return {
    ...config,
    input: config.input
      ? ensureAbsolutePath(config.input, config.configDir)
      : config.configDir,
    buildDir,
    outputDir: config.outputDir || path.join(buildDir, 'out'),
    docIdPrefix: config.docIdPrefix || '',
  };
}

function ensureAbsolutePath(filePath: string, currentDir: string): string {
  return path.normalize(
    path.isAbsolute(filePath) ? filePath : path.join(currentDir, filePath),
  );
}
