/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 **/

import {Config} from './config';
import {DoxModel, DoxCompound} from './doxygen-model';
import {DocModel, DocCompound, DocSection} from './doc-model';

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

  private static readonly sectionsOrder = [
    ['public-static-attrib', 'Public static fields'],
    ['public-static-func', 'Public static functions'],
    ['public-attrib', 'Public fields'],
    ['public-func', 'Public functions'],
    ['protected-static-attrib', 'Protected static fields'],
    ['protected-static-func', 'Protected static functions'],
    ['protected-attrib', 'Protected fields'],
    ['protected-func', 'Protected functions'],
    ['related', 'Related functions'],
  ];

  private static readonly knownSections: string[] = [
    ...Transformer.sectionsOrder.map(a => a[0]),
    'private-type',
    'private-func',
    'private-attrib',
    'private-slot',
    'private-static-func',
    'private-static-attrib',
    'public-type', // TODO: research
    'protected-type', // TODO: research
    'friend', // TODO: research
  ];

  private transformClass(doxCompound: DoxCompound) {
    const doxCompoundName = doxCompound.compoundname[0]._;
    console.log(`transforming ${doxCompoundName}`);
    const noTemplateName = doxCompoundName.split('<')[0];
    const nsp = noTemplateName.split('::');
    const compound = new DocCompound();
    compound.namespace = nsp.splice(0, nsp.length - 1).join('::');
    // TODO: make this code config driven
    if (compound.namespace === 'winrt::Microsoft::ReactNative') {
      compound.namespaceAlias = 'React';
    }
    compound.typeName = nsp[nsp.length - 1];
    compound.docId = `${this.config.prefix}${compound.typeName.toLowerCase()}`;
    this.docModel.compounds.set(compound.docId, compound);
    this.compoundMapDoxToDoc[doxCompound.$.id] = compound;
    this.compoundMapDocToDox[compound.docId] = doxCompound;

    compound.prototype = doxCompound.$.kind + ' ' + compound.typeName;

    if (Array.isArray(doxCompound.sectiondef)) {
      const sections = new Array<DocSection>(Transformer.sectionsOrder.length);

      for (const sectiondef of doxCompound.sectiondef) {
        const sectionKind = sectiondef.$.kind;
        if (!Transformer.knownSections.includes(sectionKind)) {
          throw new Error(`Unknown section kind ${sectionKind}`);
        }

        const orderedIndex = Transformer.sectionsOrder.findIndex(
          o => o[0] === sectionKind,
        );
        if (orderedIndex >= 0) {
          const section = new DocSection();
          section.title = Transformer.sectionsOrder[orderedIndex][1];
          sections[orderedIndex] = section;
        }
      }

      compound.sections = sections.filter(Boolean) as DocSection[];
    }

    console.log(compound);
  }
}
