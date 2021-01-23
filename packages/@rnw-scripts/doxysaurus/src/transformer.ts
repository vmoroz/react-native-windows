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
    for (const doxCompoundId of Object.keys(this.doxModel.compounds)) {
      const doxCompound = this.doxModel.compounds[doxCompoundId];
      switch (doxCompound.$.kind) {
        case 'struct':
        case 'class':
          this.transformClass(doxCompound);
          break;
        default:
          break;
      }
    }

    return this.docModel;
  }

  private transformClass(doxCompound: DoxCompound) {
    const doxCompoundName = doxCompound.compoundname[0]._;
    const nsp = doxCompoundName.split('::');
    const compound = new DocCompound();
    compound.namespace = nsp.splice(0, nsp.length - 1).join('::');
    compound.typeName = nsp[nsp.length - 1];
    compound.docId = `${this.config.prefix}${compound.typeName.toLowerCase()}`;
    this.docModel.compounds[compound.docId] = compound;
    this.compoundMapDoxToDoc[doxCompound.$.id] = compound;
    this.compoundMapDocToDox[compound.docId] = doxCompound;
  }
}
