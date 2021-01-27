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
import * as path from 'path';
import {log} from './logger';
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

// log.quiet = Boolean(argv.quiet);

(async () => {
  for await (const projectConfig of getProjectConfigs(argv.config)) {
    log(`[Start] processing project {${projectConfig.input}}`);
    await generateDoxygenXml(projectConfig);
    const doxModel = await DoxModel.load(projectConfig);
    const docModel = Transformer.transformToMarkdown(doxModel, projectConfig);
    await Renderer.render(docModel, projectConfig);
    log(`[Finished] processing project {${projectConfig.input}}`);

    if (process.env.DOCUSAURUS_DOCS) {
      log(
        `[Found] environment variable {DOCUSAURUS_DOCS}={${process.env.DOCUSAURUS_DOCS}}`,
      );
      log(
        `[Start] copying files to Docusaurus Docs location {${process.env.DOCUSAURUS_DOCS}}`,
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
          log(`[Copying] file to {${target}}`);
          await fsPromises.copyFile(source, target);
        }
      }
      log(
        `[Finished] copying files to Docusaurus Docs location: {${process.env.DOCUSAURUS_DOCS}}`,
      );
    } else {
      log(
        `[Cannot copy] files to Docusaurus Docs location: environment ` +
          'variable {DOCUSAURUS_DOCS} is not defined',
      );
    }
  }
})();
