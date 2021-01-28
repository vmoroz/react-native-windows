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
import {generateDoxygenXml} from './doxygen';
import {DoxModel} from './doxygen-model';
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
  for await (const config of getProjectConfigs(argv.config)) {
    log(`[Start] processing project {${config.input}}`);
    log('Project config: ', config);

    await generateDoxygenXml(config);
    const doxModel = await DoxModel.load(config);
    const docModel = Transformer.transformToMarkdown(doxModel, config);
    await renderDocFiles(docModel, config);
    await copyDocusaurusFiles(docModel);

    log(`[Finished] processing project {${config.input}}`);
  }
})();
