/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import * as xml2js from 'xml2js';
import {applyTemplateRules} from './string-template';
import {DoxMember} from '../doxygen-model';
import {toMarkdown} from '../markdown';

test('Empty brief description', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe('');
});

test('Brief description with "para" tag', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
    |<para>Test</para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe('Test');
});

test('Brief description with three "para" tags', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
    |<para>Paragraph1</para>
    |<para>Paragraph2</para>
    |<para>Paragraph3</para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`Paragraph1
      |
      |Paragraph2
      |
      |Paragraph3`),
  );
});

test('Brief description with itemized list', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
    |<itemizedlist>
    |<listitem><para>item1</para>
    |</listitem><listitem><para>item2</para>
    |</listitem><listitem><para>item3</para>
    |</listitem></itemizedlist>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`
      |
      |
      |* item1
      |* item2
      |* item3
      |
      |
      `),
  );
});

async function parse(xmlText: string) {
  const xml = await xml2js.parseStringPromise(t(xmlText), {
    explicitChildren: true,
    preserveChildrenOrder: true,
    charsAsChildren: true,
  });
  return xml.memberdef as DoxMember;
}

function t(text: string) {
  return applyTemplateRules(text, {indent: '  ', EOL: '\n'});
}
