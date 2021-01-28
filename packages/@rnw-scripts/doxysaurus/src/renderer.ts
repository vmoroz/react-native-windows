/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 **/

//
// Render documentation files defined in DocModel by applying Mustache templates.
//

import mustache from 'mustache';
import path from 'path';
import {Config} from './config';
import {DocModel} from './doc-model';
import {log} from './logger';
import {promises as fs} from 'fs';

const templateCache: {[index: string]: string} = {};

export async function renderDocFiles(docModel: DocModel, config: Config) {
  docModel.outputPath = path.join(config.output, 'out');
  await fs.mkdir(docModel.outputPath).catch(err => {
    if (err.code !== 'EEXIST') throw err;
  });

  const templatePath = path.normalize(
    path.join(__dirname, '..', 'templates', 'cpp', 'class.md'),
  );
  const template = await getCachedTemplate(templatePath);

  for (const compound of Object.values(docModel.compounds)) {
    compound.outputFileName = path.join(
      docModel.outputPath,
      `${compound.docId}.md`,
    );
    log(`[Rendering] file {${compound.outputFileName}}`);
    const outputText = mustache.render(template, compound);
    await fs.writeFile(compound.outputFileName, outputText, 'utf-8');
  }
}

async function getCachedTemplate(templatePath: string) {
  let template = templateCache[templatePath];
  if (!template) {
    template = await fs.readFile(templatePath, 'utf-8');
    templateCache[templatePath] = template;
    mustache.parse(template);
  }
  return template;
}
