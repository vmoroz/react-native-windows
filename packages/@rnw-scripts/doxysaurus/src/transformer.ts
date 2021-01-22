/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 **/

import {Config} from './config';
import {DoxModel, DoxCompound} from './doxygen-model';
import {DocModel, DocCompound} from './doc-model';

export class Transformer {
  private readonly config: Config;
  private readonly doxModel: DoxModel;
  private readonly docModel: DocModel;
  private readonly compoundMapDoxToDoc: {[index: string]: DocCompound} = {};
  private readonly compoundMapDocToDox: {[index: string]: DoxCompound} = {};

  static transformToMarkdown(doxModel: DoxModel, config: Config) {
    const transformer = new Transformer(doxModel, config);
    return transformer.transform();
  }

  private constructor(doxModel: DoxModel, config: Config) {
    this.config = config;
    this.doxModel = doxModel;
    this.docModel = new DocModel();
  }

  private transform() {
    this.config;
    this.doxModel;
    this.docModel;
    this.compoundMapDoxToDoc;
    this.compoundMapDocToDox;

    return this.docModel;
  }
}
