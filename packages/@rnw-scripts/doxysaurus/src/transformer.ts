/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 **/

import {Config} from './config';
import {
  DoxModel,
  DoxCompound,
  DoxMember,
  DoxDescription,
  DoxDescriptionElement,
  DoxRefKind,
} from './doxygen-model';
import {
  DocModel,
  DocCompound,
  DocSection,
  DocMemberOverload,
  DocMember,
} from './doc-model';
const GithubSlugger = require('github-slugger');
import * as log from 'winston';
import * as chalk from 'chalk';
import * as path from 'path';

export class Transformer {
  private readonly config: Config;
  private readonly doxModel: DoxModel;
  private readonly docModel: DocModel;
  private readonly compoundMapDocToDox: {[index: string]: DoxCompound} = {};
  readonly compoundMapDoxToDoc: {[index: string]: DocCompound} = {};
  readonly memberOverrideMap: {[index: string]: DocMemberOverload} = {};

  private readonly doxMemberMap = new Map<DocMember, DoxMember>();

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

    for (const doxCompoundId of Object.keys(this.doxModel.compounds)) {
      const doxCompound = this.doxModel.compounds[doxCompoundId];
      switch (doxCompound.$.kind) {
        case 'struct':
        case 'class':
          this.compoundToMarkdown(doxCompound);
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
    ['related', 'Related standalone functions'],
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
    compound.fileName = path.basename(doxCompound.location[0].$.file);
    this.docModel.compounds.set(compound.docId, compound);
    this.compoundMapDoxToDoc[doxCompound.$.id] = compound;
    this.compoundMapDocToDox[compound.docId] = doxCompound;

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
            this.doxMemberMap.set(member, memberdef);
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
              memberOverload = new DocMemberOverload(compound);
              memberOverload.name = overloadName;
              compoundMemberOverloads.set(overloadName, memberOverload);
              compound.memberOverloads.push(memberOverload);
            }
            if (!memberOverloads.has(overloadName)) {
              memberOverloads.set(overloadName, memberOverload);
              section.memberOverloads.push(memberOverload);
            }
            memberOverload.members.push(member);

            this.memberOverrideMap[memberdef.$.id] = memberOverload;
          }

          if (section.memberOverloads.length > 0) {
            sections[orderedIndex] = section;
            //section.memberOverloads.sort(DocMemberOverload.compareByName);
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

  private compoundToMarkdown(doxCompound: DoxCompound) {
    const compound = this.compoundMapDoxToDoc[doxCompound.$.id];
    compound.brief = this.toMarkdown(doxCompound.briefdescription);
    compound.details = this.toMarkdown(doxCompound.detaileddescription);
    compound.summary = Transformer.createSummary(
      compound.brief,
      compound.details,
    );

    compound.prototype = doxCompound.$.kind + ' ' + compound.typeName;
    if (doxCompound.basecompoundref) {
      doxCompound.basecompoundref.forEach((base, index) => {
        compound.prototype += `\n    ${index ? ',' : ':'} `;
        compound.prototype += base.$.prot + ' ';
        compound.prototype += base._.replace('< ', '<').replace(' >', '>');
      });
    }

    for (const memberOverload of compound.memberOverloads) {
      for (const member of memberOverload.members) {
        const memberDef = this.doxMemberMap.get(member);
        member.brief = this.toMarkdown(memberDef.briefdescription);
        member.details = this.toMarkdown(memberDef.detaileddescription);
        member.summary = Transformer.createSummary(
          member.brief,
          member.details,
        );

        let m: string[] = [];
        if (memberDef.templateparamlist) {
          m.push('template<');
          if (
            memberDef.templateparamlist.length > 0 &&
            memberDef.templateparamlist[0].param
          ) {
            memberDef.templateparamlist[0].param.forEach((param, argn) => {
              m = m.concat(argn === 0 ? [] : ',');
              m = m.concat([this.toMarkdownNoLink(param.type)]);
              // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
              if (param.defval && param.defval[0]._) {
                m = m.concat([' = ', param.defval[0]._]);
              }
            });
          }
          m.push('>  \n');
        }
        m = m.concat([memberDef.$.prot, ': ']); // public, private, ...
        m = m.concat(memberDef.$.static === 'yes' ? ['static', ' '] : []);
        m = m.concat(memberDef.$.virt === 'virtual' ? ['virtual', ' '] : []);
        let typeMarkdown = this.toMarkdownNoLink(memberDef.type);
        if (typeMarkdown.trim() !== '') {
          typeMarkdown = typeMarkdown.replace(/\s+/g, ' ');
          m = m.concat(
            typeMarkdown,
            typeMarkdown.endsWith('&') || typeMarkdown.endsWith('*') ? '' : ' ',
          );
        }
        m = m.concat(memberDef.$.explicit === 'yes' ? ['explicit', ' '] : []);
        m = m.concat(memberDef.name[0]._);
        const argsstring = this.toMarkdownNoLink(memberDef.argsstring);
        if (argsstring.trim() !== '') {
          m = m.concat(argsstring.replace('=', ' = '));
        }
        m = m.concat(';');

        member.prototype = m.join('');

        if (!memberOverload.summary) {
          if (memberOverload.name === '(constructor)') {
            memberOverload.summary = `constructs the ${'[`' +
              compound.typeName +
              '`](' +
              compound.docId +
              ')'}`;
          } else if (memberOverload.name === '(destructor)') {
            memberOverload.summary = `destroys the ${'[`' +
              compound.typeName +
              '`](' +
              compound.docId +
              ')'}`;
          } else {
            memberOverload.summary = member.summary.replace(/^[A-Z]/, match =>
              match.toLowerCase(),
            );
          }
        }
      }
    }
  }

  private static createSummary(brief: string, details: string) {
    // set from brief or first paragraph of details
    let summary = Transformer.trim(brief);
    if (!summary) {
      summary = Transformer.trim(details);
      if (summary) {
        const firstParagraph = summary.split('\n', 1)[0];
        if (firstParagraph) {
          summary = firstParagraph;
        }
      }
    }
    if (!summary) {
      summary = '&nbsp;';
    }
    return summary;
  }

  private static trim(text: string) {
    return text.replace(/^[\s\t\r\n]+|[\s\t\r\n]+$/g, '');
  }

  private toMarkdown(desc: DoxDescription) {
    return MarkdownTransformer.transform(this, desc);
  }

  private toMarkdownNoLink(desc: DoxDescription) {
    return MarkdownTransformer.transform(this, desc, true);
  }
}

class MarkdownTransformer {
  private readonly context: DoxDescriptionElement[] = [];
  private readonly output: string[] = [];

  static transform(
    transformer: Transformer,
    desc: DoxDescription,
    noLink: boolean = false,
  ) {
    const mdTransformer = new MarkdownTransformer(transformer, noLink);
    mdTransformer.w(desc);
    return mdTransformer.output.join('');
  }

  private constructor(
    private readonly transformer: Transformer,
    private noLink: boolean,
  ) {}

  // eslint-disable-next-line complexity
  private transformElement(element: DoxDescriptionElement) {
    switch (element['#name']) {
      case 'ref':
        return this.refLink(
          MarkdownTransformer.transform(this.transformer, element.$$, true),
          element.$.refid,
          element.$.kindref,
        );
      case '__text__':
        return this.autoLinks(element._);
      case 'para':
        return this.w(element.$$, '\n\n');
      case 'emphasis':
        return this.w('*', element.$$, '*');
      case 'bold':
        return this.w('**', element.$$, '**');
      case 'parametername':
        return this.w('`', element.$$, '` ');
      case 'computeroutput': {
        const noLinkPrev = this.noLink;
        this.noLink = true;
        this.w('`', element.$$, '`');
        this.noLink = noLinkPrev;
        return this;
      }
      case 'parameterlist':
        if (element.$.kind === 'exception') {
          return this.w('\n### Exceptions\n', element.$$, '\n\n');
        } else {
          return this.w('\n### Parameters\n', element.$$, '\n\n');
        }
      case 'parameteritem':
        return this.w('* ', element.$$, '\n');
      case 'programlisting': {
        const noLinkPrev = this.noLink;
        this.noLink = true;
        this.w('\n```cpp\n', element.$$, '```\n');
        this.noLink = noLinkPrev;
        return this;
      }
      case 'codeline':
        return this.w(element.$$, '\n');
      case 'orderedlist':
        this.context.push(element);
        this.w('\n\n', element.$$, '\n');
        this.context.pop();
        return this;
      case 'itemizedlist':
        return this.w('\n\n', element.$$, '\n');
      case 'listitem':
        return this.w(
          this.context.length > 0 &&
            this.context[this.context.length - 1]['#name'] === 'orderedlist'
            ? '1. '
            : '* ',
          element.$$,
          '\n',
        );
      case 'sp':
        return this.w(' ', element.$$);
      case 'heading':
        return this.w('## ', element.$$);
      case 'xrefsect':
        return this.w('\n> ', element.$$);
      case 'simplesect':
        if (element.$.kind === 'attention') {
          return this.w('> ', element.$$);
        } else if (element.$.kind === 'return') {
          return this.w('\n### Returns\n', element.$$);
        } else if (element.$.kind === 'see') {
          return this.w('**See also**: ', element.$$);
        } else {
          log.warn(
            `${chalk.red(`element.$.kind=${element.$.kind}`)}: not supported.`,
          );
          return this;
        }
      case 'formula':
        let s = this.trim(element._ || '');
        if (s.startsWith('$') && s.endsWith('$')) {
          return this.w(s);
        }
        if (s.startsWith('\\[') && s.endsWith('\\]')) {
          s = this.trim(s.substring(2, s.length - 2));
        }
        return this.w('\n$$\n' + s + '\n$$\n');
      case 'preformatted':
        return this.w('\n<pre>', element.$$, '</pre>\n');
      case 'sect1':
      case 'sect2':
      case 'sect3':
        this.context.push(element);
        this.w('\n', element.$$, '\n');
        this.context.pop();
        return this;
      case 'title':
        let level = 0;
        if (this.context.length > 0) {
          level = Number(
            (this.context[this.context.length - 1]['#name'] || '0').slice(-1),
          );
        }
        return this.w('\n', '#'.repeat(level), ' ', element._, '\n\n');
      case 'mdash':
        return this.w('&mdash;');
      case 'ndash':
        return this.w('&neath;');
      case 'linebreak':
        return this.w('<br/>');
      case 'ulink':
        return this.link(
          MarkdownTransformer.transform(this.transformer, element.$$, true),
          element.$.url,
        );
      case 'xreftitle':
        return this.w('**', element.$$, ':** ');
      case 'row':
        this.w(
          '\n',
          this.escapeRow(
            MarkdownTransformer.transform(this.transformer, element.$$),
          ),
        );
        if ((element.$$[0] as DoxDescriptionElement).$.thead === 'yes') {
          element.$$.forEach((_, i) => {
            this.w(i ? ' | ' : '\n', '---------');
          });
        }
        return this;
      case 'entry':
        return this.w(
          this.escapeCell(
            MarkdownTransformer.transform(this.transformer, element.$$),
          ),
          '|',
        );
      case 'highlight':
      case 'table':
      case 'parameterdescription':
      case 'parameternamelist':
      case 'xrefdescription':
      case 'verbatim':
      case 'hruler':
      case undefined:
        return this.w(element.$$);

      default:
        log.warn(
          `${chalk.red(
            `element['#name'=${element['#name']}]`,
          )}: not supported.`,
        );
        return this;
    }
  }

  private refLink(text: string, refId: string, refKind: DoxRefKind) {
    switch (refKind) {
      case 'compound':
        const compound = this.transformer.compoundMapDoxToDoc[refId];
        return this.link(text, compound.docId || '', true);
      case 'member':
        const memberOverload = this.transformer.memberOverrideMap[refId];
        return this.link(
          text,
          (memberOverload.compound.docId || '') + (memberOverload.anchor || ''),
          true,
        );
      default:
        log.warn(`Unknown refkind '${refKind}'`);
        return this;
    }
  }

  private link(text: string, href: string, isCode = false) {
    return this.noLink
      ? this.w(text)
      : isCode
      ? this.w('[`', text, '`](', href, ')')
      : this.w('[', text, '](', href, ')');
  }

  private autoLinks(text?: string) {
    if (!this.noLink && text) {
      text = this.applyStandardLibLinks(text);
      // text = this.applyIdlGeneratedLinks(text);
    }
    return this.w(text);
  }

  stdLibRefs: {[index: string]: string} = {
    'std::intializer_list':
      'https://en.cppreference.com/w/cpp/utility/initializer_list',
    'std::vector': 'https://en.cppreference.com/w/cpp/container/vector',
  };

  private applyStandardLibLinks(text: string) {
    return text.replace(
      /(std::\w+)(<\w+>)?(::\w+\(\)|::operator\[\])?/g,
      (match, p1, _, p3) => {
        const ref = this.stdLibRefs[p1];
        if (ref) {
          if (p3) {
            if (p3.endsWith('()')) {
              return `[\`${match}\`](${ref}/${p3.slice(2, -2)})`;
            } else if (p3.endsWith('operator[]')) {
              return `[\`${match}\`](${ref}/operator_at)`;
            }
          }
          return `[\`${match}\`](${ref})`;
        } else {
          return match;
        }
      },
    );
  }

  private trim(text: string) {
    return text.replace(/^[\s\t\r\n]+|[\s\t\r\n]+$/g, '');
  }

  private escapeRow(text: string) {
    return text.replace(/\s*\|\s*$/, '');
  }

  private escapeCell(text: string) {
    return text
      .replace(/^[\n]+|[\n]+$/g, '') // trim CRLF
      .replace('/|/g', '\\|') // escape the pipe
      .replace(/\n/g, '<br/>'); // escape CRLF
  }

  private w(...items: DoxDescription[]): MarkdownTransformer {
    for (const item of items) {
      switch (typeof item) {
        case 'string':
          this.output.push(item);
          break;
        case 'object':
          if (Array.isArray(item)) {
            for (const element of <DoxDescription[]>item) {
              this.w(element);
            }
          } else {
            this.transformElement(item as DoxDescriptionElement);
          }
          break;
        case 'undefined':
          break;
        default:
          throw new Error(`Unexpected object type: ${typeof item}`);
      }
    }

    return this;
  }
}
