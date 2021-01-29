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
  const docModel = new DocModel();

  const docIdToDoxCompound = new Map<string, DoxCompound | undefined>();
  const doxIdToDocCompound = new Map<string, DocCompound | undefined>();

  const memberOverloadMap: {[index: string]: DocMemberOverload} = {};
  const memberOverloadToCompound = new Map<DocMemberOverload, DocCompound>();
  const doxMemberMap = new Map<DocMember, DoxMember>();

  const types = new Set<string>(config.types ?? []);
  const namespaceAliases = new Map<string, string[] | undefined>(
    config.namespaceAliases ?? [],
  );
  const knownSections = new Map<string, string>(config.sections ?? []);

  const linkResolver: LinkResolver = {
    stdTypeLinks: {
      linkPrefix: config.stdTypeLinks?.linkPrefix ?? '',
      linkMap: new Map<string, string>(config.stdTypeLinks?.linkMap ?? []),
    },
    idlTypeLinks: {
      linkPrefix: config.idlTypeLinks?.linkPrefix ?? '',
      linkMap: new Map<string, string>(config.idlTypeLinks?.linkMap ?? []),
    },
    resolveCompoundId: (doxCompoundId: string): DocCompound | undefined => {
      return doxIdToDocCompound.get(doxCompoundId);
    },
    resolveMemberId: (
      doxMemberId: string,
    ): [DocCompound | undefined, DocMemberOverload | undefined] => {
      const memberOverload = memberOverloadMap[doxMemberId];
      const compound = memberOverloadToCompound.get(memberOverload);
      return [compound, memberOverload];
    },
  };

  for (const doxCompoundId of Object.keys(doxModel.compounds)) {
    const doxCompound = doxModel.compounds[doxCompoundId];
    switch (doxCompound.$.kind) {
      case 'struct':
      case 'class':
        transformClass(doxCompound);
        break;
      default:
        break;
    }
  }

  for (const compound of Object.values(docModel.compounds)) {
    compoundToMarkdown(compound);
  }

  return docModel;

  // eslint-disable-next-line complexity
  function transformClass(doxCompound: DoxCompound) {
    const doxCompoundName = doxCompound.compoundname[0]._;
    log(`[Transforming] ${doxCompoundName}`);
    const noTemplateName = doxCompoundName.split('<')[0];
    const nsp = noTemplateName.split('::');
    const compound = new DocCompound();
    compound.namespace = nsp.splice(0, nsp.length - 1).join('::');
    compound.namespaceAliases = [];
    compound.namespaceAliases = namespaceAliases.get(compound.namespace) ?? [];
    compound.name = nsp[nsp.length - 1];
    if (!types.has(compound.name)) {
      log(`[Skipped] {${doxCompoundName}}: not in config.types`);
      return;
    }
    compound.docId = `${config.prefix}${compound.name.toLowerCase()}`;
    compound.codeFileName = path.basename(doxCompound.location[0].$.file);
    docModel.compounds[compound.docId] = compound;
    doxIdToDocCompound.set(doxCompound.$.id, compound);
    docIdToDoxCompound.set(compound.docId, doxCompound);

    const compoundMemberOverloads = new Map<string, DocMemberOverload>();

    const slugger = new GithubSlugger();

    if (Array.isArray(doxCompound.sectiondef)) {
      const visibleSections: {[index: string]: DocSection} = {};

      for (const sectionDef of doxCompound.sectiondef) {
        const sectionKind = sectionDef.$.kind;
        const sectionName = knownSections.get(sectionKind);

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
          doxMemberMap.set(member, memberDef);
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
            memberOverloadToCompound.set(memberOverload, compound);
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

          memberOverloadMap[memberDef.$.id] = memberOverload;

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

  function compoundToMarkdown(compound: DocCompound) {
    const doxCompound = docIdToDoxCompound.get(compound.docId);
    compound.brief = toMarkdown(doxCompound?.briefdescription, linkResolver);
    compound.details = toMarkdown(
      doxCompound?.detaileddescription,
      linkResolver,
    );
    compound.summary = createSummary(compound.brief, compound.details);

    compound.declaration = createCompoundDeclaration();
    for (const section of compound.sections) {
      for (const memberOverload of section.memberOverloads) {
        for (const member of memberOverload.members) {
          const memberDef = doxMemberMap.get(member);
          member.brief = toMarkdown(memberDef.briefdescription, linkResolver);
          member.details = toMarkdown(
            memberDef.detaileddescription,
            linkResolver,
          );
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
              memberOverload.summary = member.summary.replace(/^[A-Z]/, match =>
                match.toLowerCase(),
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
      sb.write(doxCompound?.$.kind ?? '', ' ', compound.name);
      doxCompound?.basecompoundref?.forEach((base, index) => {
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
          sb.write(toMarkdown(param.type));
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
      sb.write(trim(toMarkdown(memberDef.argsstring)).replace('=', ' = '));
      sb.write(';');

      return sb.toString();

      function writeType() {
        let memberType = toMarkdown(memberDef.type);
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
}

function toMarkdown(desc: DoxDescription, linkResolver?: LinkResolver) {
  const context: DoxDescriptionElement[] = [];
  const sb = new StringBuilder();
  write(desc);
  return sb.toString();

  // eslint-disable-next-line complexity
  function transformElement(element: DoxDescriptionElement): void {
    switch (element['#name']) {
      case 'ref':
        return refLink(element);
      case '__text__':
        return autoLinks(element._);
      case 'para':
        return write(element.$$, '\n\n');
      case 'emphasis':
        return write('*', element.$$, '*');
      case 'bold':
        return write('**', element.$$, '**');
      case 'parameterlist':
        return write('\n### Parameters\n', element.$$, '\n\n');
      case 'parameteritem':
        return write('* ', element.$$, '\n');
      case 'parametername':
        return writeCode('`', element.$$, '` ');
      case 'computeroutput':
        return writeCode('`', element.$$, '`');
      case 'programlisting':
        return writeCode('\n```cpp\n', element.$$, '```\n');
      case 'codeline':
        return write(element.$$, '\n');
      case 'orderedlist':
        return writeWithContext(element, '\n\n', element.$$, '\n');
      case 'itemizedlist':
        return write('\n\n', element.$$, '\n');
      case 'listitem':
        return write(
          last(context)?.['#name'] === 'orderedlist' ? '1. ' : '* ',
          element.$$,
          '\n',
        );
      case 'sp':
        return write(' ', element.$$);
      case 'heading':
        return write('## ', element.$$);
      case 'xrefsect':
        return write('\n> ', element.$$);
      case 'simplesect':
        if (element.$.kind === 'attention') {
          return write('> ', element.$$);
        } else if (element.$.kind === 'return') {
          return write('\n### Returns\n', element.$$);
        } else if (element.$.kind === 'see') {
          return write('**See also**: ', element.$$);
        } else {
          log(`Warning: [element.$.kind=${element.$.kind}]: not supported.`);
          return;
        }
      case 'formula':
        let s = trim(element._ || '');
        if (s.startsWith('$') && s.endsWith('$')) {
          return write(s);
        }
        if (s.startsWith('\\[') && s.endsWith('\\]')) {
          s = trim(s.substring(2, s.length - 2));
        }
        return write('\n$$\n' + s + '\n$$\n');
      case 'preformatted':
        return writeCode('\n<pre>', element.$$, '</pre>\n');
      case 'sect1':
      case 'sect2':
      case 'sect3':
        return writeWithContext(element, '\n', element.$$, '\n');
      case 'title':
        const level = Number((last(context)?.['#name'] || '0').slice(-1));
        return write('\n', '#'.repeat(level), ' ', element._, '\n\n');
      case 'mdash':
        return write('&mdash;');
      case 'ndash':
        return write('&neath;');
      case 'linebreak':
        return write('<br/>');
      case 'ulink':
        return link(toMarkdown(element.$$), element.$.url);
      case 'xreftitle':
        return write('**', element.$$, ':** ');
      case 'row':
        write('\n', escapeRow(toMarkdown(element.$$, linkResolver)));
        if ((element.$$[0] as DoxDescriptionElement).$.thead === 'yes') {
          element.$$.forEach((_, i) => {
            write(i ? ' | ' : '\n', '---------');
          });
        }
        break;
      case 'entry':
        return write(escapeCell(toMarkdown(element.$$, linkResolver)), '|');
      case 'highlight':
      case 'table':
      case 'parameterdescription':
      case 'parameternamelist':
      case 'xrefdescription':
      case 'verbatim':
      case 'hruler':
      case undefined:
        return write(element.$$);

      default:
        log(
          `Warning: [element[['#name'=${element['#name']}]]]: not supported.`,
        );
    }
  }

  function refLink(element: DoxDescriptionElement): void {
    const text = toMarkdown(element.$$);

    if (!linkResolver) {
      return write(text);
    }

    if (element.$.kindref === 'compound') {
      const compound = linkResolver.resolveCompoundId(element.$.refid);
      if (compound) {
        return linkCode(text, compound.docId);
      }
    } else if (element.$.kindref === 'member') {
      const [compound, memberOverload] = linkResolver.resolveMemberId(
        element.$.refid,
      );
      if (compound) {
        return linkCode(text, compound.docId + memberOverload?.anchor);
      }
    } else {
      log(`Warning: Unknown kindref={${element.$.kindref}}`);
    }

    write(text);
  }

  function link(text: string, href: string) {
    if (linkResolver) {
      write('[', text, '](', href, ')');
    } else {
      write(text);
    }
  }

  function linkCode(text: string, href: string) {
    if (linkResolver) {
      write('[`', text, '`](', href, ')');
    } else {
      write(text);
    }
  }

  function autoLinks(text?: string) {
    if (text && linkResolver) {
      text = applyStandardLibLinks(text, linkResolver.stdTypeLinks);
      text = applyIdlGeneratedLinks(text, linkResolver.idlTypeLinks);
    }
    write(text);
  }

  function applyStandardLibLinks(text: string, stdTypeLinks: TypeLinks) {
    return text.replace(
      /(std::\w+)(<\w+>)?(::\w+\(\)|::operator\[\])?/g,
      (match, p1, _, p3) => {
        const typeLink = stdTypeLinks.linkMap.get(p1);
        if (typeLink) {
          const ref = stdTypeLinks.linkPrefix + typeLink;
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

  function applyIdlGeneratedLinks(text: string, idlTypeLinks: TypeLinks) {
    return text.replace(/(\w+)(::\w+(\(\))?)?/g, (match, p1) => {
      const ref = idlTypeLinks.linkMap.get(p1);
      if (ref) {
        return `[\`${match}\`](${idlTypeLinks.linkPrefix + ref})`;
      } else {
        return match;
      }
    });
  }

  function escapeRow(text: string): string {
    return text.replace(/\s*\|\s*$/, '');
  }

  function escapeCell(text: string): string {
    return text
      .replace(/^[\n]+|[\n]+$/g, '') // trim CRLF
      .replace('/|/g', '\\|') // escape the pipe
      .replace(/\n/g, '<br/>'); // escape CRLF
  }

  function writeCode(...items: DoxDescription[]) {
    const oldLinkResolver = linkResolver;
    linkResolver = undefined;
    write(...items);
    linkResolver = oldLinkResolver;
  }

  function writeWithContext(
    element: DoxDescriptionElement,
    ...items: DoxDescription[]
  ) {
    context.push(element);
    write(...items);
    context.pop();
  }

  function write(...items: DoxDescription[]): void {
    for (const item of items) {
      if (typeof item === 'string') {
        sb.write(item);
      } else if (typeof item === 'object') {
        if (Array.isArray(item)) {
          for (const element of <DoxDescription[]>item) {
            write(element);
          }
        } else {
          transformElement(item as DoxDescriptionElement);
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

  writeIf(arg: string, condition: boolean) {
    if (condition) {
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
