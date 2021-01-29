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

import yargs from 'yargs';
import {log} from './logger';
import {getProjectConfigs} from './config';
import {generateDoxygenXml} from './doxygen';
import {DoxModel} from './doxygen-model';
import {transformToMarkdown} from './transformer';
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
    log: {
      alias: 'l',
      describe: 'log file',
      type: 'string',
    },
  })
  .version(false)
  .help(false).argv;

(async () => {
  try {
    log.init({quiet: argv.quiet, logFile: argv.log});

    for await (const config of getProjectConfigs(argv.config)) {
      log(`[Start] processing project {${config.input}}`);
      log('Project config: ', config);

      await generateDoxygenXml(config);
      const doxModel = await DoxModel.load(config);
      const docModel = transformToMarkdown(doxModel, config);
      const files = await renderDocFiles(docModel, config);
      await copyDocusaurusFiles(files);

      log(`[Finished] processing project {${config.input}}`);
    }
  } catch (err) {
    try {
      log(`Error: `, err);
    } catch (logError) {
      console.error(logError);
    }

    process.exit(1);
  }
})();
