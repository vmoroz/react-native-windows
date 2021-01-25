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
import * as path from 'path';
import {getProjectConfigs} from './config';
import {generateDoxygenXml} from './doxygen';
import {DoxModel} from './doxygen-model';
import {Transformer} from './transformer';
import {Renderer} from './renderer';

import * as fs from 'fs';
const fsPromises = fs.promises;

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

    if (process.env.DOCUSAURUS_DOCS) {
      log.verbose(
        `${chalk.greenBright('Found')} environment variable ${chalk.cyanBright(
          'DOCUSAURUS_DOCS',
        )} = ${chalk.cyanBright(process.env.DOCUSAURUS_DOCS)}`,
      );
      log.verbose(
        `${chalk.greenBright(
          'Copying',
        )} files to Docusaurus Docs location: ${chalk.cyanBright(
          process.env.DOCUSAURUS_DOCS,
        )}`,
      );
      for (const compoundEntry of docModel.compounds) {
        const compound = compoundEntry[1];
        if (compound.docId) {
          const source = path.join(
            projectConfig.output,
            'out',
            compound.docId + '.md',
          );
          const target = path.join(
            process.env.DOCUSAURUS_DOCS,
            compound.docId + '.md',
          );
          log.verbose(
            `${chalk.greenBright('Copying')} file to ${chalk.cyanBright(
              target,
            )}`,
          );
          await fsPromises.copyFile(source, target);
        }
      }
      log.verbose(
        `${chalk.greenBright(
          'Finished copying',
        )} files to Docusaurus Docs location: ${chalk.cyanBright(
          process.env.DOCUSAURUS_DOCS,
        )}`,
      );
    } else {
      log.verbose(
        `${chalk.greenBright(
          'Not copying',
        )} files to Docusaurus Docs location: environment variable ${chalk.cyanBright(
          'DOCUSAURUS_DOCS',
        )} is not defined`,
      );
    }
  }
})();
