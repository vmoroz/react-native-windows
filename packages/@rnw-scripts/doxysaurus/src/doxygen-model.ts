/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import {Config} from './config';
import * as chalk from 'chalk';
import * as path from 'path';
import * as fs from 'fs';
const fsPromises = fs.promises;
import * as xml2js from 'xml2js';
import * as log from 'winston';

export class DoxModel {
  compounds: {[index: string]: DoxCompound} = {};

  constructor(public config: Config) {}

  static async load(config: Config): Promise<DoxModel> {
    const model = new DoxModel(config);
    const indexPath = path.join(config.output, 'xml', 'index.xml');
    log.verbose(`Loading index ${chalk.cyanBright(indexPath)}`);
    const indexText = await fsPromises.readFile(indexPath, 'utf8');
    const indexXml = <IndexRootType>await xml2js.parseStringPromise(indexText);
    await model.loadCompounds(indexXml);
    return model;
  }

  private async loadCompounds(indexXml: IndexRootType) {
    for (const compound of indexXml.doxygenindex.compound) {
      switch (compound.$.kind) {
        case 'class':
        case 'struct':
          await this.loadCompound(compound);
          break;
        default:
          break;
      }
    }
  }

  private async loadCompound(compound: IndexCompoundType) {
    const compoundPath = path.join(
      this.config.output,
      'xml',
      `${compound.$.refid}.xml`,
    );
    log.verbose(`Loading compound ${chalk.cyanBright(compoundPath)}`);
    const compoundText = await fsPromises.readFile(compoundPath, 'utf8');
    const compoundXml = <CompoundRootType>await xml2js.parseStringPromise(
      compoundText,
      {
        explicitChildren: true,
        preserveChildrenOrder: true,
        charsAsChildren: true,
      },
    );
    const compoundDef = compoundXml.doxygen.compounddef[0];
    this.compounds[compoundDef.$.id] = compoundDef;
  }
}

interface CompoundRootType {
  doxygen: {compounddef: DoxCompound[]};
}

export interface DoxCompound {
  $: {id: string; kind: DoxCompoundKind};
  compoundname: {_: string}[]; // there is only one entry
}

interface IndexRootType {
  doxygenindex: {compound: IndexCompoundType[]};
}

interface IndexCompoundType {
  $: {
    refid: string;
    kind: DoxCompoundKind;
  };
}

export type DoxCompoundKind =
  | 'class'
  | 'struct'
  | 'union'
  | 'interface'
  | 'protocol'
  | 'category'
  | 'exception'
  | 'service'
  | 'singleton'
  | 'module'
  | 'type'
  | 'file'
  | 'namespace'
  | 'group'
  | 'page'
  | 'example'
  | 'dir';