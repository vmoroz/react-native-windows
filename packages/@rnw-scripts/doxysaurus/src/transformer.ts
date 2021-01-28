/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * Code in this file is partially based on the moxygen project code
 * which in turn is based on the doxygen2md project.
 * https://github.com/sourcey/moxygen
 * https://github.com/pferdinand/doxygen2md
 * Copyright for the moxygen and doxygen2md code:
 * Copyright (c) 2016 Philippe FERDINAND
 * Copyright (c) 2016 Kam Low
 *
 * @format
 **/

//
// Transforms Doxygen documentation model to the Markdown based model to be used by Docusaurus service.
//

import {Config} from './config';
import {
  DoxModel,
  DoxCompound,
  DoxMember,
  DoxDescription,
  DoxDescriptionElement,
} from './doxygen-model';
import {
  DocModel,
  DocCompound,
  DocSection,
  DocMemberOverload,
  DocMember,
} from './doc-model';
import GithubSlugger from 'github-slugger';
import * as path from 'path';
import {log} from './logger';

interface TypeLinks {
  linkPrefix: string;
  linkMap: Map<string, string>;
}

interface LinkResolver {
  stdTypeLinks: TypeLinks;
  idlTypeLinks: TypeLinks;
  resolveCompoundId(doxCompoundId: string): DocCompound | undefined;
  resolveMemberId(
    doxMemberId: string,
  ): [DocCompound | undefined, DocMemberOverload | undefined];
}

export function transformToMarkdown(doxModel: DoxModel, config: Config) {
  class Transformer {
    readonly config: Config;
    private readonly doxModel: DoxModel;
    private readonly docModel: DocModel;
    private readonly compoundMapDocToDox: {[index: string]: DoxCompound} = {};
    readonly knownSections: Map<string, string>;
    readonly compoundMapDoxToDoc: {
      [index: string]: DocCompound | undefined;
    } = {};
    readonly memberOverloadMap: {[index: string]: DocMemberOverload} = {};
    readonly memberOverloadToCompound = new Map<
      DocMemberOverload,
      DocCompound
    >();

    private readonly doxMemberMap = new Map<DocMember, DoxMember>();

    readonly types: Set<string>;
    readonly namespaceAliases: Map<string, string[] | undefined>;
    readonly stdTypeLinks: TypeLinks;
    readonly idlTypeLinks: TypeLinks;

    static transformToMarkdown(doxModel: DoxModel, config: Config) {
      const transformer = new Transformer(doxModel, config);
      return transformer.transform();
    }

    private constructor(doxModel: DoxModel, config: Config) {
      this.config = config;
      this.doxModel = doxModel;
      this.docModel = new DocModel();
      this.knownSections = new Map<string, string>(config.sections ?? []);
      this.types = new Set<string>(config.types ?? []);
      this.namespaceAliases = new Map<string, string[] | undefined>(
        config.namespaceAliases ?? [],
      );
      this.stdTypeLinks = {
        linkPrefix: config.stdTypeLinks?.linkPrefix ?? '',
        linkMap: new Map<string, string>(config.stdTypeLinks?.linkMap ?? []),
      };
      this.idlTypeLinks = {
        linkPrefix: config.idlTypeLinks?.linkPrefix ?? '',
        linkMap: new Map<string, string>(config.idlTypeLinks?.linkMap ?? []),
      };
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

      for (const compound of Object.values(this.docModel.compounds)) {
        this.compoundToMarkdown(compound);
      }

      return this.docModel;
    }

    // eslint-disable-next-line complexity
    private transformClass(doxCompound: DoxCompound) {
      const doxCompoundName = doxCompound.compoundname[0]._;
      log(`[Transforming] ${doxCompoundName}`);
      const noTemplateName = doxCompoundName.split('<')[0];
      const nsp = noTemplateName.split('::');
      const compound = new DocCompound();
      compound.namespace = nsp.splice(0, nsp.length - 1).join('::');
      compound.namespaceAliases = [];
      compound.namespaceAliases =
        this.namespaceAliases.get(compound.namespace) ?? [];
      compound.name = nsp[nsp.length - 1];
      if (!this.types.has(compound.name)) {
        log(`[Skipped] {${doxCompoundName}}: not in config.types`);
        return;
      }
      compound.docId = `${this.config.prefix}${compound.name.toLowerCase()}`;
      compound.codeFileName = path.basename(doxCompound.location[0].$.file);
      this.docModel.compounds[compound.docId] = compound;
      this.compoundMapDoxToDoc[doxCompound.$.id] = compound;
      this.compoundMapDocToDox[compound.docId] = doxCompound;

      const compoundMemberOverloads = new Map<string, DocMemberOverload>();

      const slugger = new GithubSlugger();

      if (Array.isArray(doxCompound.sectiondef)) {
        const visibleSections: {[index: string]: DocSection} = {};

        for (const sectionDef of doxCompound.sectiondef) {
          const sectionKind = sectionDef.$.kind;
          const sectionName = this.knownSections.get(sectionKind);

          if (typeof sectionName === 'undefined') {
            throw new Error(`Unknown section kind ${sectionKind}`);
          }

          if (sectionName === '<not visible>') {
            continue;
          }

          let section: DocSection;
          if (sectionName === '<user defined>') {
            section = new DocSection();
            section.name = sectionDef.header[0]._;
            visibleSections[section.name] = section;
          } else {
            section = visibleSections[sectionName];
            if (typeof section === 'undefined') {
              section = new DocSection();
              section.name = sectionName;
              visibleSections[sectionName] = section;
            }
          }

          const memberOverloads = new Map<string, DocMemberOverload>();

          for (const memberDef of sectionDef.memberdef) {
            const memberName = memberDef.name[0]._;
            const member = new DocMember();
            member.line = Number(memberDef.location[0].$.line);
            this.doxMemberMap.set(member, memberDef);
            member.name = memberName;
            const overloadName =
              memberName === compound.name
                ? '(constructor)'
                : memberName === '~' + compound.name
                ? '(destructor)'
                : memberName === 'operator='
                ? 'assignment operator='
                : memberName === 'operator=='
                ? 'equal operator=='
                : memberName === 'operator!='
                ? 'not equal operator!='
                : memberName === 'operator[]'
                ? 'subscript operator[]'
                : memberName;
            let memberOverload = compoundMemberOverloads.get(overloadName);
            if (typeof memberOverload === 'undefined') {
              memberOverload = new DocMemberOverload();
              this.memberOverloadToCompound.set(memberOverload, compound);
              memberOverload.name = overloadName;
              memberOverload.line = member.line;
              compoundMemberOverloads.set(overloadName, memberOverload);
            }
            if (!memberOverloads.has(overloadName)) {
              memberOverloads.set(overloadName, memberOverload);
              section.memberOverloads.push(memberOverload);
            }
            memberOverload.members.push(member);
            memberOverload.line = Math.min(memberOverload.line, member.line);

            this.memberOverloadMap[memberDef.$.id] = memberOverload;

            if (section.line === 0) {
              section.line = member.line;
            } else {
              section.line = Math.min(section.line, member.line);
            }
          }

          section.memberOverloads.sort((a, b) => a.line - b.line);
        }

        const sections: DocSection[] = Object.values(visibleSections);
        // Put deprecated section to the end
        sections.forEach(
          s =>
            (s.line = s.name.includes('Deprecated')
              ? Number.MAX_SAFE_INTEGER
              : s.line),
        );
        sections.sort((a, b) => a.line - b.line);
        compound.sections = sections;
      }

      for (const section of compound.sections) {
        for (const memberOverload of section.memberOverloads) {
          memberOverload.anchor = '#' + slugger.slug(memberOverload.name);
        }
      }

      log('[Compound] dump: ', compound);
    }

    private compoundToMarkdown(compound: DocCompound) {
      const toMarkdownNoLink = (desc: DoxDescription) =>
        this.toMarkdownNoLink(desc);

      const doxCompound = this.compoundMapDocToDox[compound.docId];
      compound.brief = this.toMarkdown(doxCompound.briefdescription);
      compound.details = this.toMarkdown(doxCompound.detaileddescription);
      compound.summary = createSummary(compound.brief, compound.details);

      compound.declaration = createCompoundDeclaration();
      for (const section of compound.sections) {
        for (const memberOverload of section.memberOverloads) {
          for (const member of memberOverload.members) {
            const memberDef = this.doxMemberMap.get(member);
            member.brief = this.toMarkdown(memberDef.briefdescription);
            member.details = this.toMarkdown(memberDef.detaileddescription);
            member.summary = createSummary(member.brief, member.details);

            if (!memberOverload.summary) {
              if (memberOverload.name === '(constructor)') {
                memberOverload.summary = `constructs the ${'[`' +
                  compound.name +
                  '`](' +
                  compound.docId +
                  ')'}`;
              } else if (memberOverload.name === '(destructor)') {
                memberOverload.summary = `destroys the ${'[`' +
                  compound.name +
                  '`](' +
                  compound.docId +
                  ')'}`;
              } else {
                memberOverload.summary = member.summary.replace(
                  /^[A-Z]/,
                  match => match.toLowerCase(),
                );
              }
            }

            member.declaration = createMemberDeclaration(memberDef);
          }
        }
      }

      function createSummary(brief: string, details: string) {
        let summary = trim(brief);
        if (!summary) summary = trim(details).split('\n', 1)[0];
        if (!summary) summary = '&nbsp;';
        return summary;
      }

      function createCompoundDeclaration() {
        const sb = new StringBuilder();
        sb.write(doxCompound.$.kind, ' ', compound.name);
        doxCompound.basecompoundref?.forEach((base, index) => {
          sb.write('\n    ', index ? ', ' : ': ');
          sb.write(base.$.prot, ' ');
          sb.write(base._.replace('< ', '<').replace(' >', '>'));
        });
        return sb.toString();
      }

      function createMemberDeclaration(memberDef: DoxMember) {
        const sb = new StringBuilder();
        if (memberDef.templateparamlist) {
          sb.write('template<');
          memberDef.templateparamlist[0].param?.forEach((param, index) => {
            sb.writeIf(',', index !== 0);
            sb.write(toMarkdownNoLink(param.type));
            if (param.defval?.[0]._) {
              sb.write(' = ', param.defval[0]._);
            }
          });
          sb.write('>  \n');
        }
        sb.write(memberDef.$.prot, ': '); // public, private, ...
        sb.writeIf('static ', memberDef.$.static === 'yes');
        sb.writeIf('virtual ', memberDef.$.virt === 'virtual');
        writeType();
        sb.writeIf('explicit ', memberDef.$.explicit === 'yes');
        sb.write(memberDef.name[0]._);
        sb.write(
          trim(toMarkdownNoLink(memberDef.argsstring)).replace('=', ' = '),
        );
        sb.write(';');

        return sb.toString();

        function writeType() {
          let memberType = toMarkdownNoLink(memberDef.type);
          if (trim(memberType) !== '') {
            memberType = memberType.replace(/\s+/g, ' ');
            sb.write(memberType);
            sb.write(
              memberType.endsWith('&') || memberType.endsWith('*') ? '' : ' ',
            );
          }
        }
      }
    }

    resolveCompoundId(doxCompoundId: string): DocCompound | undefined {
      return this.compoundMapDoxToDoc[doxCompoundId];
    }

    resolveMemberId(
      doxMemberId: string,
    ): [DocCompound | undefined, DocMemberOverload | undefined] {
      const memberOverload = this.memberOverloadMap[doxMemberId];
      const compound = this.memberOverloadToCompound.get(memberOverload);
      return [compound, memberOverload];
    }

    private toMarkdown(desc: DoxDescription) {
      return MarkdownTransformer.transform(desc, this);
    }

    private toMarkdownNoLink(desc: DoxDescription) {
      return MarkdownTransformer.transform(desc);
    }
  }

  return Transformer.transformToMarkdown(doxModel, config);
}

class MarkdownTransformer {
  private readonly context: DoxDescriptionElement[] = [];
  private readonly sb = new StringBuilder();

  static transform(desc: DoxDescription, linkResolver?: LinkResolver) {
    const mdTransformer = new MarkdownTransformer(linkResolver);
    mdTransformer.write(desc);
    return mdTransformer.sb.toString();
  }

  private constructor(private linkResolver?: LinkResolver) {}

  // eslint-disable-next-line complexity
  private transformElement(element: DoxDescriptionElement): void {
    switch (element['#name']) {
      case 'ref':
        return this.refLink(element);
      case '__text__':
        return this.autoLinks(element._);
      case 'para':
        return this.write(element.$$, '\n\n');
      case 'emphasis':
        return this.write('*', element.$$, '*');
      case 'bold':
        return this.write('**', element.$$, '**');
      case 'parameterlist':
        return this.write('\n### Parameters\n', element.$$, '\n\n');
      case 'parameteritem':
        return this.write('* ', element.$$, '\n');
      case 'parametername':
        return this.writeCode('`', element.$$, '` ');
      case 'computeroutput':
        return this.writeCode('`', element.$$, '`');
      case 'programlisting':
        return this.writeCode('\n```cpp\n', element.$$, '```\n');
      case 'codeline':
        return this.write(element.$$, '\n');
      case 'orderedlist':
        return this.writeWithContext(element, '\n\n', element.$$, '\n');
      case 'itemizedlist':
        return this.write('\n\n', element.$$, '\n');
      case 'listitem':
        return this.write(
          last(this.context)?.['#name'] === 'orderedlist' ? '1. ' : '* ',
          element.$$,
          '\n',
        );
      case 'sp':
        return this.write(' ', element.$$);
      case 'heading':
        return this.write('## ', element.$$);
      case 'xrefsect':
        return this.write('\n> ', element.$$);
      case 'simplesect':
        if (element.$.kind === 'attention') {
          return this.write('> ', element.$$);
        } else if (element.$.kind === 'return') {
          return this.write('\n### Returns\n', element.$$);
        } else if (element.$.kind === 'see') {
          return this.write('**See also**: ', element.$$);
        } else {
          log(`Warning: [element.$.kind=${element.$.kind}]: not supported.`);
          return;
        }
      case 'formula':
        let s = trim(element._ || '');
        if (s.startsWith('$') && s.endsWith('$')) {
          return this.write(s);
        }
        if (s.startsWith('\\[') && s.endsWith('\\]')) {
          s = trim(s.substring(2, s.length - 2));
        }
        return this.write('\n$$\n' + s + '\n$$\n');
      case 'preformatted':
        return this.writeCode('\n<pre>', element.$$, '</pre>\n');
      case 'sect1':
      case 'sect2':
      case 'sect3':
        return this.writeWithContext(element, '\n', element.$$, '\n');
      case 'title':
        const level = Number((last(this.context)?.['#name'] || '0').slice(-1));
        return this.write('\n', '#'.repeat(level), ' ', element._, '\n\n');
      case 'mdash':
        return this.write('&mdash;');
      case 'ndash':
        return this.write('&neath;');
      case 'linebreak':
        return this.write('<br/>');
      case 'ulink':
        return this.link(
          MarkdownTransformer.transform(element.$$),
          element.$.url,
        );
      case 'xreftitle':
        return this.write('**', element.$$, ':** ');
      case 'row':
        this.write(
          '\n',
          this.escapeRow(
            MarkdownTransformer.transform(element.$$, this.linkResolver),
          ),
        );
        if ((element.$$[0] as DoxDescriptionElement).$.thead === 'yes') {
          element.$$.forEach((_, i) => {
            this.write(i ? ' | ' : '\n', '---------');
          });
        }
        break;
      case 'entry':
        return this.write(
          this.escapeCell(
            MarkdownTransformer.transform(element.$$, this.linkResolver),
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
        return this.write(element.$$);

      default:
        log(
          `Warning: [element[['#name'=${element['#name']}]]]: not supported.`,
        );
    }
  }

  private refLink(element: DoxDescriptionElement): void {
    const text = MarkdownTransformer.transform(element.$$);

    if (!this.linkResolver) {
      return this.write(text);
    }

    if (element.$.kindref === 'compound') {
      const compound = this.linkResolver.resolveCompoundId(element.$.refid);
      if (compound) {
        return this.linkCode(text, compound.docId);
      }
    } else if (element.$.kindref === 'member') {
      const [compound, memberOverload] = this.linkResolver.resolveMemberId(
        element.$.refid,
      );
      if (compound) {
        return this.linkCode(text, compound.docId + memberOverload?.anchor);
      }
    } else {
      log(`Warning: Unknown kindref={${element.$.kindref}}`);
    }

    this.write(text);
  }

  private link(text: string, href: string) {
    if (this.linkResolver) {
      this.write('[', text, '](', href, ')');
    } else {
      this.write(text);
    }
  }

  private linkCode(text: string, href: string) {
    if (this.linkResolver) {
      this.write('[`', text, '`](', href, ')');
    } else {
      this.write(text);
    }
  }

  private autoLinks(text?: string) {
    if (text && this.linkResolver) {
      text = this.applyStandardLibLinks(text, this.linkResolver);
      text = this.applyIdlGeneratedLinks(text, this.linkResolver);
    }
    this.write(text);
  }

  private applyStandardLibLinks(text: string, linkResolver: LinkResolver) {
    const stdTypeLinks = linkResolver.stdTypeLinks;
    return text.replace(
      /(std::\w+)(<\w+>)?(::\w+\(\)|::operator\[\])?/g,
      (match, p1, _, p3) => {
        const link = stdTypeLinks.linkMap.get(p1);
        if (link) {
          const ref = stdTypeLinks.linkPrefix + link;
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

  private applyIdlGeneratedLinks(text: string, linkResolver: LinkResolver) {
    return text.replace(/(\w+)(::\w+(\(\))?)?/g, (match, p1) => {
      const idlTypeLinks = linkResolver.idlTypeLinks;
      const ref = idlTypeLinks.linkMap.get(p1);
      if (ref) {
        return `[\`${match}\`](${idlTypeLinks.linkPrefix + ref})`;
      } else {
        return match;
      }
    });
  }

  private escapeRow(text: string): string {
    return text.replace(/\s*\|\s*$/, '');
  }

  private escapeCell(text: string): string {
    return text
      .replace(/^[\n]+|[\n]+$/g, '') // trim CRLF
      .replace('/|/g', '\\|') // escape the pipe
      .replace(/\n/g, '<br/>'); // escape CRLF
  }

  private writeCode(...items: DoxDescription[]) {
    const linkResolver = this.linkResolver;
    this.linkResolver = undefined;
    this.write(...items);
    this.linkResolver = linkResolver;
  }

  private writeWithContext(
    element: DoxDescriptionElement,
    ...items: DoxDescription[]
  ) {
    this.context.push(element);
    this.write(...items);
    this.context.pop();
  }

  private write(...items: DoxDescription[]): void {
    for (const item of items) {
      if (typeof item === 'string') {
        this.sb.write(item);
      } else if (typeof item === 'object') {
        if (Array.isArray(item)) {
          for (const element of <DoxDescription[]>item) {
            this.write(element);
          }
        } else {
          this.transformElement(item as DoxDescriptionElement);
        }
      } else if (typeof item !== 'undefined') {
        throw new Error(`Unexpected object type: ${typeof item}`);
      }
    }
  }
}

class StringBuilder {
  private readonly parts: string[] = [];

  write(...args: string[]) {
    for (const arg of args) {
      this.parts.push(arg);
    }
  }

  writeIf(arg: string, cond: boolean) {
    if (cond) {
      this.parts.push(arg);
    }
  }

  toString() {
    return this.parts.join('');
  }
}

function last<T>(arr?: T[]): T | undefined {
  return arr ? arr[arr.length - 1] : undefined;
}

function trim(text: string) {
  return text.replace(/^[\s\t\r\n]+|[\s\t\r\n]+$/g, '');
}
