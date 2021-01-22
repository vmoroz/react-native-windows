/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

// The Doxysaurus uses Doxygen to generate documentation based on code,
// and then generates markdown files to be used by the Docusaurus service.

import * as yargs from 'yargs';
import * as log from 'winston';
import {getProjectConfigs} from './config';
import {generateDoxygenXml} from './doxygen';
import {DoxModel} from './doxygen-model';
import {Transformer} from './transformer';

const argv = yargs
  .options({
    config: {
      alias: 'c',
      describe: 'config file',
      type: 'string',
      demandOption: true,
    },
    quiet: {
      alias: 'q',
      describe: 'quiet mode',
      type: 'boolean',
    },
  })
  .version(false)
  .help(false).argv;

log.add(
  new log.transports.Console({
    format: log.format.combine(log.format.colorize(), log.format.simple()),
    level: argv.quiet ? undefined : 'verbose',
  }),
);

(async () => {
  for await (const projectConfig of getProjectConfigs(argv.config)) {
    await generateDoxygenXml(projectConfig);
    const doxModel = await DoxModel.load(projectConfig);
    const docModel = Transformer.transformToMarkdown(doxModel, projectConfig);
    docModel;
    // TODO: generate output from the docModel
  }
})();
