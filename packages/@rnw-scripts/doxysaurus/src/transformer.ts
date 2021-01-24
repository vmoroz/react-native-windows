/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 **/

import {Config} from './config';
import {DoxModel, DoxCompound} from './doxygen-model';
import {
  DocModel,
  DocCompound,
  DocSection,
  DocMemberOverload,
  DocMember,
} from './doc-model';
const GithubSlugger = require('github-slugger');

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
    ['public-attrib', 'Public fields'],
    ['public-func', 'Public functions'],
    ['public-static-attrib', 'Public static fields'],
    ['public-static-func', 'Public static functions'],
    ['protected-attrib', 'Protected fields'],
    ['protected-func', 'Protected functions'],
    ['protected-static-attrib', 'Protected static fields'],
    ['protected-static-func', 'Protected static functions'],
    ['related', 'Related functions'],
    ['user-defined', 'User Defined'],
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

    const compoundMemberOverloads = new Map<string, DocMemberOverload>();

    const slugger = new GithubSlugger();

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

          const memberOverloads = new Map<string, DocMemberOverload>();

          for (const memberdef of sectiondef.memberdef) {
            const memberName = memberdef.name[0]._;
            const member = new DocMember();
            member.name = memberName;
            const overloadName =
              memberName === compound.typeName
                ? '(constructor)'
                : memberName === '~' + compound.typeName
                ? '(destructor)'
                : memberName;
            let memberOverload = compoundMemberOverloads.get(overloadName);
            // Disable the false positive
            // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
            if (!memberOverload) {
              memberOverload = new DocMemberOverload();
              memberOverload.name = overloadName;
              compoundMemberOverloads.set(overloadName, memberOverload);
              compound.memberOverloads.push(memberOverload);
            }
            if (!memberOverloads.has(overloadName)) {
              memberOverloads.set(overloadName, memberOverload);
              section.memberOverloads.push(memberOverload);
            }
            memberOverload.members.push(member);
          }

          if (section.memberOverloads.length > 0) {
            sections[orderedIndex] = section;
            section.memberOverloads.sort(DocMemberOverload.compareByName);
          }
        }
      }

      compound.sections = sections.filter(Boolean) as DocSection[];
    }

    compound.memberOverloads.sort(DocMemberOverload.compareByName);
    for (const memberOverload of compound.memberOverloads) {
      memberOverload.anchor = '#' + slugger.slug(memberOverload.name);
    }

    console.dir(compound, {depth: null});
  }
}
