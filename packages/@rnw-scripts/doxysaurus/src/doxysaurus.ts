/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

//
// The Doxysaurus uses Doxygen to generate documentation based on code,
// and then generates markdown files to be used by the Docusaurus service.
//

import * as yargs from 'yargs';
import * as log from 'winston';
import * as chalk from 'chalk';
import {getProjectConfigs} from './config';
import {generateDoxygenXml} from './doxygen';
import {DoxModel} from './doxygen-model';
import {Transformer} from './transformer';
import {Renderer} from './renderer';

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
    log.verbose(
      `${chalk.greenBright('Start')} processing project ${chalk.cyanBright(
        projectConfig.input,
      )}`,
    );
    await generateDoxygenXml(projectConfig);
    const doxModel = await DoxModel.load(projectConfig);
    const docModel = Transformer.transformToMarkdown(doxModel, projectConfig);
    await Renderer.render(docModel, projectConfig);
    log.verbose(
      `${chalk.greenBright('Finished')} processing project ${chalk.cyanBright(
        projectConfig.input,
      )}`,
    );
  }
})();
