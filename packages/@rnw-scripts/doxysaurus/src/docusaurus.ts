/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

//
// Copy generated documentation files to Docusaurus location.
//

import * as path from 'path';
import {promises as fs} from 'fs';
import {log} from './logger';
import {DocModel} from './doc-model';

export async function copyDocusaurusFiles(docModel: DocModel) {
  const docsPath = process.env.DOCUSAURUS_DOCS;
  if (!docsPath) {
    log(`[Not found] environment var {DOCUSAURUS_DOCS} to copy files`);
    return;
  }

  log(`[Found] environment var {DOCUSAURUS_DOCS} = {${docsPath}}`);
  log(`[Start] copying files to Docusaurus Docs {${docsPath}}`);

  for (const compound of Object.values(docModel.compounds)) {
    if (compound.outputFileName) {
      const target = path.join(
        docsPath,
        path.basename(compound.outputFileName),
      );
      log(`[Copying] file to {${target}}`);
      await fs.copyFile(compound.outputFileName, target);
    }
  }

  log(`[Finished] copying files to Docusaurus Docs: {${docsPath}}`);
}
