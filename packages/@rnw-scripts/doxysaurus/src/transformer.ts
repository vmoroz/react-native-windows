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
import GithubSlugger from 'github-slugger';
import * as path from 'path';
import {log} from './logger';

interface TypeLinks {
  linkPrefix: string;
  linkMap: Map<string, string>;
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

            createMemberDeclaration(member, memberDef);
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

      function createMemberDeclaration(
        member: DocMember,
        memberDef: DoxMember,
      ) {
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
            log(`Warning: [element.$.kind=${element.$.kind}]: not supported.`);
            return this;
          }
        case 'formula':
          let s = trim(element._ || '');
          if (s.startsWith('$') && s.endsWith('$')) {
            return this.w(s);
          }
          if (s.startsWith('\\[') && s.endsWith('\\]')) {
            s = trim(s.substring(2, s.length - 2));
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
          log(
            `Warning: [element[['#name'=${element['#name']}]]]: not supported.`,
          );
          return this;
      }
    }

    private refLink(text: string, refId: string, refKind: DoxRefKind) {
      switch (refKind) {
        case 'compound':
          const compound = this.transformer.compoundMapDoxToDoc[refId];
          if (compound) {
            return this.link(text, compound.docId, true);
          } else {
            return this.w(text);
          }
        case 'member':
          const memberOverload = this.transformer.memberOverloadMap[refId];
          const memberCompound = this.transformer.memberOverloadToCompound.get(
            memberOverload,
          );
          return this.link(
            text,
            memberCompound.docId + memberOverload.anchor,
            true,
          );
        default:
          log(`Warning: Unknown refkind={${refKind}}`);
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
        text = this.applyIdlGeneratedLinks(text);
      }
      return this.w(text);
    }

    private applyStandardLibLinks(text: string) {
      const stdTypeLinks = this.transformer.stdTypeLinks;
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

    private applyIdlGeneratedLinks(text: string) {
      return text.replace(/(\w+)(::\w+(\(\))?)?/g, (match, p1) => {
        const idlTypeLinks = this.transformer.idlTypeLinks;
        const ref = idlTypeLinks.linkMap.get(p1);
        if (ref) {
          return `[\`${match}\`](${idlTypeLinks.linkPrefix + ref})`;
        } else {
          return match;
        }
      });
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

  return Transformer.transformToMarkdown(doxModel, config);
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

function trim(text: string) {
  return text.replace(/^[\s\t\r\n]+|[\s\t\r\n]+$/g, '');
}
