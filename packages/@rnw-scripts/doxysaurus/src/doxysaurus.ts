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
import {log} from './logger';
import {getProjectConfigs} from './config';
import {DoxModel} from './doxygen-model';
import {generateDoxygenXml} from './doxygen';
import {Transformer} from './transformer';
import {renderDocFiles} from './renderer';
import {copyDocusaurusFiles} from './docusaurus';

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

log.quiet = argv.quiet;

(async () => {
  for await (const projectConfig of getProjectConfigs(argv.config)) {
    log(`[Start] processing project {${projectConfig.input}}`);

    await generateDoxygenXml(projectConfig);
    const doxModel = await DoxModel.load(projectConfig);
    const docModel = Transformer.transformToMarkdown(doxModel, projectConfig);
    await renderDocFiles(docModel, projectConfig);
    await copyDocusaurusFiles(docModel);

    log(`[Finished] processing project {${projectConfig.input}}`);
  }
})();
