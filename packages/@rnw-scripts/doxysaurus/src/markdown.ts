/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * This code is partially based on moxygen and doxygen2md code.
 * The copyrights of moxygen and doxygen2md projects:
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
  operatorMap: Map<string, string>;
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
        return writeText(element._);
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
        if (element.$.kind === 'param') {
          return write(index ? '\n\n' : '', '### Parameters\n', element.$$);
        } else if (element.$.kind === 'retval') {
          return write(index ? '\n\n' : '', '### Return values\n', element.$$);
        } else {
          return log(
            `Warning: Unknown parameterlist kind {${element.$.kind}}.`,
          );
        }
      case 'parameteritem':
        return write('\n* ', element.$$);
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
          return write(index ? '\n\n' : '', '### Returns\n\n', element.$$);
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
      case 'ref':
        return refLink(element);
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
      case 'anchor':
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
      } else {
        return write('`', text, '`');
      }
    } else if (element.$.kindref === 'member') {
      const [compound, memberOverload] = linkResolver.resolveMemberId(
        element.$.refid,
      );
      if (compound) {
        return linkCode(text, compound.docId + memberOverload?.anchor);
      } else {
        return write('`', text, '`');
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

  function writeText(text?: string) {
    if (text) {
      if (text.trim() === '') {
        if (text.includes(' ')) {
          write(' ');
        }
      } else {
        // Do auto-linking for known types
        if (linkResolver) {
          applyStandardLibLinks(text, linkResolver.stdTypeLinks);
          // text = applyIdlGeneratedLinks(text, linkResolver.idlTypeLinks);
        } else {
          write(text);
        }
      }
    }
  }

  // We support the following syntax for standard library types and functions:
  // - It must start 'std::' and follow by a type name.
  // - Then we optionally may have template arguments in < >
  // - Then we optionally may have '::' followed by method name or an operator.
  // eslint-disable-next-line complexity
  function applyStandardLibLinks(text: string, stdTypeLinks: TypeLinks) {
    const typeExpr = regex('y')`
      (?<typeName>std::\w+)     // standard lib type name
      |(?<ltBracket><)          // start template args
      |(?<gtBracket>>)          // end template args
      |::(?<member>
           (?<operator>operator // list of all member operators
             (\[\]
             |\(\)
             |,
             |->\*
             |-[-=>]?
             |\+[+=]?
             |\*=?
             |/=?
             |%=?
             |\^=?
             |~=?
             |<<?=?>?
             |>>?=?
             |&&?=?
             |\|\|?=?
             |==?
             |!=?
             )
           )
         |(?<method>\w+)(\(\))? // method to match
         )`;
    let index = 0;
    let templateDepth = 0;
    let wasInTemplateArgs = false;
    let inTemplateArgs = false;
    let typeLink: string | undefined;
    let inMember = false;
    let inCode = false;

    for (let i = 0; i < text.length; ) {
      typeExpr.lastIndex = i;
      const match = typeExpr.exec(text);
      if (match) {
        if (match.groups?.typeName) {
          write(text.substring(index, match.index));
          typeLink = stdTypeLinks.linkMap.get(match.groups.typeName);
          if (typeLink) {
            [wasInTemplateArgs, inTemplateArgs] = [inTemplateArgs, false];
            if (wasInTemplateArgs) {
              write('`');
            }
            writeCodeLink(match[0], stdTypeLinks.linkPrefix, typeLink);
            [wasInTemplateArgs, inTemplateArgs] = [false, wasInTemplateArgs];
            if (text.startsWith('<', typeExpr.lastIndex)) {
              inTemplateArgs = true;
            } else if (text.startsWith('::', typeExpr.lastIndex)) {
              inMember = true;
            } else {
              typeLink = undefined;
            }
            if (inTemplateArgs) {
              write('`');
            }
          } else {
            write(match[0]);
          }
          index = typeExpr.lastIndex;
        } else if (match.groups?.ltBracket) {
          if (inTemplateArgs) {
            ++templateDepth;
          }
        } else if (match.groups?.gtBracket) {
          if (inTemplateArgs) {
            --templateDepth;
            if (templateDepth === 0) {
              inTemplateArgs = false;
              write(text.substring(index, typeExpr.lastIndex));
              index = typeExpr.lastIndex;
              if (text.startsWith('::', index)) {
                inMember = true;
                inCode = true;
              } else {
                typeLink = undefined;
                write('`');
              }
            }
          }
        } else if (inMember) {
          inMember = false;
          if (match.groups?.member) {
            if (match.groups.method) {
              if (!inCode) {
                write('`');
              }
              write('::`');
              writeCodeLink(
                match.groups.member,
                stdTypeLinks.linkPrefix,
                typeLink ?? '',
                '/',
                match.groups.method,
              );
            } else if (match.groups.operator) {
              const operatorLink = stdTypeLinks.operatorMap.get(
                match.groups.operator,
              );
              if (operatorLink) {
                if (!inCode) {
                  write('`');
                }
                write('::`');
                writeCodeLink(
                  match.groups.operator,
                  stdTypeLinks.linkPrefix,
                  typeLink ?? '',
                  '/',
                  operatorLink,
                );
              } else {
                if (!inCode) {
                  write('`');
                }
                write('::', match.groups.operator, '`');
              }
            }
            index = typeExpr.lastIndex;
          }
          inCode = false;
          typeLink = undefined;
        }
        i = typeExpr.lastIndex;
      } else {
        ++i;
      }
    }

    write(text.substring(index));
  }

  function writeCodeLink(label: string, ...linkParts: string[]) {
    write('[`', label, '`](', ...linkParts, ')');
  }

  // let index = 0;
  // let codeStarted = false;

  // const match = typeExpr.exec(text);
  // if (match) {
  //   if (match[1]) {
  //     const typeLink = stdTypeLinks.linkMap.get(match[1]);
  //     if (typeLink) {
  //       write('[`', match[0], '`](', typeLink, ')');
  //     } else {
  //       codeStarted = true;
  //       write('`', match[0]);
  //     }
  //     if (text.startsWith('<', typeExpr.lastIndex)) {
  //       ++linkedTemplateDepth;
  //       ++typeExpr.lastIndex;
  //       if (!codeStarted) {
  //         write('`');
  //       }
  //       write('<');
  //     } else {
  //       if (codeStarted) {
  //         codeStarted = false;
  //         write('`');
  //       }
  //     }
  //     index = typeExpr.lastIndex;
  //   } else if (linkedTemplateDepth) {
  //     if (match[0] === '<') {
  //       ++linkedTemplateDepth;
  //     } else if (match[0] === '>') {
  //       --linkedTemplateDepth;
  //     }
  //   }
  // }
  // return text.replace(
  //   /(std::\w+)(<\w+>)?(::\w+\(\)|::operator\[\])?/g,
  //   (match, p1, _, p3) => {
  //     const typeLink = stdTypeLinks.linkMap.get(p1);
  //     if (typeLink) {
  //       const ref = stdTypeLinks.linkPrefix + typeLink;
  //       if (p3) {
  //         if (p3.endsWith('()')) {
  //           return `[\`${match}\`](${ref}/${p3.slice(2, -2)})`;
  //         } else if (p3.endsWith('operator[]')) {
  //           return `[\`${match}\`](${ref}/operator_at)`;
  //         }
  //       }
  //       return `[\`${match}\`](${ref})`;
  //     } else {
  //       return match;
  //     }
  //   },
  // );
  //}

  // function applyIdlGeneratedLinks(text: string, idlTypeLinks: TypeLinks) {
  //   return text.replace(/(\w+)(::\w+(\(\))?)?/g, (match, p1) => {
  //     const ref = idlTypeLinks.linkMap.get(p1);
  //     if (ref) {
  //       return `[\`${match}\`](${idlTypeLinks.linkPrefix + ref})`;
  //     } else {
  //       return match;
  //     }
  //   });
  // }

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
            // Remove all non-meaningful white spaces
            .filter(
              element =>
                typeof element !== 'object' ||
                element['#name'] !== '__text__' ||
                (element._ && (element._.trim() || element._.includes(' '))),
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

// Converted to TypeScript from https://stackoverflow.com/a/62153852
interface TaggedTemplate {
  (strings: TemplateStringsArray, ...values: string[]): RegExp;
}
function regex(arg0: string): TaggedTemplate;
function regex(strings: TemplateStringsArray, ...values: string[]): RegExp;
function regex(
  arg0: string | TemplateStringsArray,
  ...args: string[]
): RegExp | TaggedTemplate {
  function cleanup(str: string) {
    // remove whitespace, single and multi-line comments
    return str.replace(/\s+|\/\/.*|\/\*[\s\S]*?\*\//g, '');
  }

  function escape(str: string) {
    // escape regular expression
    return str.replace(/[-.*+?^${}()|[\]\\]/g, '\\$&');
  }

  function create(
    flags: string,
    strings: TemplateStringsArray,
    ...values: string[]
  ) {
    let pattern = '';
    for (let i = 0; i < values.length; ++i) {
      pattern += cleanup(strings.raw[i]); // strings are cleaned up
      pattern += escape(values[i]); // values are escaped
    }
    pattern += cleanup(strings.raw[values.length]);
    return RegExp(pattern, flags);
  }

  if (Array.isArray(arg0)) {
    // used as a template tag (no flags)
    return create('', arg0 as TemplateStringsArray, ...args);
  }

  // used as a function (with flags)
  return create.bind(void 0, arg0 as string);
}
