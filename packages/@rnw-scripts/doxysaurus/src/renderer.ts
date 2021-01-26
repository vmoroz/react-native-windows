/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 **/

import {Config} from './config';
import {DocModel} from './doc-model';
import * as mustache from 'mustache';
import * as path from 'path';
import * as fs from 'fs';
const fsPromises = fs.promises;

export class Renderer {
  private readonly config: Config;
  private readonly docModel: DocModel;
  private static readonly templateCache: {[index: string]: string} = {};

  static async render(docModel: DocModel, config: Config) {
    const renderer = new Renderer(docModel, config);
    await renderer.renderModel();
  }

  private constructor(docModel: DocModel, config: Config) {
    this.config = config;
    this.docModel = docModel;
  }

  private async renderModel() {
    const outDir = path.join(this.config.output, 'out');
    await fsPromises.mkdir(outDir).catch(err => {
      if (err.code !== 'EEXIST') throw err;
    });

    const templatePath = path.normalize(
      path.join(__dirname, '..', 'templates', 'cpp', 'class.md'),
    );
    const template = await Renderer.getTemplate(templatePath);

    for (const docCompound of this.docModel.compounds) {
      const outputText = mustache.render(template, docCompound[1]);
      const outputPath = path.join(outDir, `${docCompound[0]}.md`);
      console.log(`write file: ${outputPath}`);
      await fsPromises.writeFile(outputPath, outputText, 'utf-8');
    }
  }

  private static async getTemplate(templatePath: string) {
    let template = Renderer.templateCache[templatePath];
    if (!template) {
      template = await fsPromises.readFile(templatePath, 'utf-8');
      Renderer.templateCache[templatePath] = template;
      mustache.parse(template);
    }
    return template;
  }
}
