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
// Transforms Doxygen descriptions to markdown.
//

import {DoxDescription, DoxDescriptionElement} from './doxygen-model';
import {DocCompound, DocMemberOverload} from './doc-model';
import {log} from './logger';

export interface TypeLinks {
  linkPrefix: string;
  linkMap: Map<string, string>;
}

export interface LinkResolver {
  stdTypeLinks: TypeLinks;
  idlTypeLinks: TypeLinks;
  resolveCompoundId(doxCompoundId: string): DocCompound | undefined;
  resolveMemberId(
    doxMemberId: string,
  ): [DocCompound | undefined, DocMemberOverload | undefined];
}

export function toMarkdown(desc: DoxDescription, linkResolver?: LinkResolver) {
  const context: DoxDescriptionElement[] = [];
  let indent = 0;

  const sb = new StringBuilder();
  write(desc);
  return sb.toString().trim();

  // eslint-disable-next-line complexity
  function transformElement(
    element: DoxDescriptionElement,
    index: number,
  ): void {
    switch (element['#name']) {
      case '__text__':
        return autoLinks(element._);
      case 'ref':
        return refLink(element);
      case 'sp':
        return write(' ');
      case 'nonbreakablespace':
        return write('&nbsp;');
      case 'ensp':
        return write('&ensp;');
      case 'emsp':
        return write('&emsp;');
      case 'thinsp':
        return write('&thinsp;');
      case 'emphasis':
        return write('*', element.$$, '*');
      case 'bold':
        return write('**', element.$$, '**');
      case 'mdash':
        return write('&mdash;');
      case 'ndash':
        return write('&ndash;');
      case 'linebreak':
        return write('<br/>');
      case 'para':
        return write(
          index ? '\n\n' : '',
          ' '.repeat(index ? indent : 0),
          element.$$,
        );
      case 'orderedlist':
      case 'itemizedlist':
        return writeWithContext(element, index ? '\n' : '', element.$$);
      case 'listitem':
        const itemBullet =
          last(context)?.['#name'] === 'orderedlist' ? '1. ' : '* ';
        return writeWithIndent(
          itemBullet.length,
          index ? '\n' : '',
          ' '.repeat(indent),
          itemBullet,
          element.$$,
        );
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
        let s = (element._ || '').trim();
        if (s.startsWith('$') && s.endsWith('$')) {
          return write(s);
        }
        if (s.startsWith('\\[') && s.endsWith('\\]')) {
          s = s.substring(2, s.length - 2).trim();
        }
        return write('\n$$\n' + s + '\n$$\n');
      case 'preformatted':
        return writeCode('\n<pre>', element.$$, '</pre>\n');
      case 'sect1':
      case 'sect2':
      case 'sect3':
      case 'sect4':
        return writeWithContext(element, index ? '\n\n' : '', element.$$);
      case 'title':
        const level = Number((last(context)?.['#name'] || '0').slice(-1));
        return write('#'.repeat(level), ' ', element._);
      case 'heading':
        sb.removeLastIf(' ');
        return write('#'.repeat(Number(element.$.level || 0)), ' ', element.$$);
      case 'hruler':
        return write('---');
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
    if (text) {
      if (text.trim() === '') {
        if (text.includes(' ')) {
          write(' ');
        }
      } else {
        if (linkResolver) {
          text = applyStandardLibLinks(text, linkResolver.stdTypeLinks);
          text = applyIdlGeneratedLinks(text, linkResolver.idlTypeLinks);
        }
        write(text);
      }
    }
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

  function writeWithIndent(indentBy: number, ...items: DoxDescription[]) {
    indent += indentBy;
    write(...items);
    indent -= indentBy;
  }

  function write(...items: DoxDescription[]): void {
    for (const item of items) {
      if (typeof item === 'string') {
        sb.write(item);
      } else if (typeof item === 'object') {
        if (Array.isArray(item)) {
          (item as DoxDescriptionElement[])
            .filter(
              element =>
                typeof element !== 'object' ||
                element['#name'] !== '__text__' ||
                !element._ ||
                element._.trim() ||
                element._.includes(' '),
            )
            .forEach((element, index) => {
              transformElement(element, index);
            });
        } else {
          transformElement(item as DoxDescriptionElement, 0);
        }
      } else if (typeof item !== 'undefined') {
        throw new Error(`Unexpected object type: ${typeof item}`);
      }
    }
  }
}

export class StringBuilder {
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

  removeLastIf(text: string) {
    if (last(this.parts) === text) {
      this.parts.pop();
    }
  }

  toString() {
    return this.parts.join('');
  }
}

function last<T>(arr?: T[]): T | undefined {
  return arr ? arr[arr.length - 1] : undefined;
}
