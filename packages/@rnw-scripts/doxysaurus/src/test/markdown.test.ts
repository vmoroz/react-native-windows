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

//
// Test conversion from Doxygen XML to Markdown.
//
// These tests are using the string-template to have more natural string look and feel.
// The leading pipeline '|' means an end of previous line and the of a new line.
// All other end-of-line characters are removed from the string.
//
// The XML spaces are significant in Doxygen XML docs.
// We keep them as we parse XML.
// Using the pipeline '|' character we indent the XML text for better readability.
//

test('Empty brief description', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe('');
});

test('One paragraph', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<para>Test</para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe('Test');
});

test('Three paragraphs', async () => {
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

test('Itemized list', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<para><itemizedlist>
        |<listitem><para>item1</para>
        |</listitem><listitem><para>item2</para>
        |</listitem><listitem><para>item3</para>
        |</listitem></itemizedlist>
        |</para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`* item1
      |* item2
      |* item3
      `),
  );
});

test('Itemized list with paragraphs', async () => {
  const memberDef = await parse(`
  |<memberdef>
  |  <briefdescription>
      |<para><itemizedlist>
      |<listitem><para>item1 para1</para>
      |<para>item1 para2</para>
      |<para>item1 para3</para>
      |</listitem><listitem><para>item2</para>
      |</listitem><listitem><para>item3</para>
      |</listitem></itemizedlist>
      |</para>
  |  </briefdescription>
  |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`* item1 para1
      |
      |  item1 para2
      |
      |  item1 para3
      |* item2
      |* item3
      `),
  );
});

test('Ordered list', async () => {
  const memberDef = await parse(`
  |<memberdef>
  |  <briefdescription>
      |<para><orderedlist>
      |<listitem><para>item1</para>
      |</listitem><listitem><para>item2</para>
      |</listitem><listitem><para>item3</para>
      |</listitem></orderedlist>
      |</para>
  |  </briefdescription>
  |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`1. item1
      |1. item2
      |1. item3
      `),
  );
});

test('Ordered list with paragraphs', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<para><orderedlist>
        |<listitem><para>item1 para1</para>
        |<para>item1 para2</para>
        |<para>item1 para3</para>
        |</listitem><listitem><para>item2</para>
        |</listitem><listitem><para>item3</para>
        |</listitem></orderedlist>
        |</para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`1. item1 para1
      |
      |   item1 para2
      |
      |   item1 para3
      |1. item2
      |1. item3
      `),
  );
});

test('Nested ordered list', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<para><orderedlist>
        |<listitem><para>item1<orderedlist>
        |<listitem><para>item1 item1</para>
        |</listitem><listitem><para>item1 item2</para>
        |</listitem></orderedlist>
        |</para>
        |</listitem><listitem><para>item2<itemizedlist>
        |<listitem><para>item2 item1</para>
        |</listitem><listitem><para>item2 item2</para>
        |</listitem></itemizedlist>
        |</para>
        |</listitem><listitem><para>item3</para>
        |</listitem></orderedlist>
        |</para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`1. item1
      |   1. item1 item1
      |   1. item1 item2
      |1. item2
      |   * item2 item1
      |   * item2 item2
      |1. item3
      `),
  );
});

test('Nested itemized list', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<para><itemizedlist>
        |<listitem><para>item1<orderedlist>
        |<listitem><para>item1 item1</para>
        |</listitem><listitem><para>item1 item2</para>
        |</listitem></orderedlist>
        |</para>
        |</listitem><listitem><para>item2<itemizedlist>
        |<listitem><para>item2 item1</para>
        |</listitem><listitem><para>item2 item2</para>
        |</listitem></itemizedlist>
        |</para>
        |</listitem><listitem><para>item3 </para>
        |</listitem></itemizedlist>
        |</para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`* item1
      |  1. item1 item1
      |  1. item1 item2
      |* item2
      |  * item2 item1
      |  * item2 item2
      |* item3
      `),
  );
});

test('Itemized list between paragraphs', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<para>text1</para>
        |<para><itemizedlist>
        |<listitem><para>item1</para>
        |</listitem><listitem><para>item2</para>
        |</listitem><listitem><para>item3</para>
        |</listitem></itemizedlist>
        |</para>
        |<para>text2 </para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`text1
      |
      |* item1
      |* item2
      |* item3
      |
      |text2
      `),
  );
});

test('Bold and emphasis', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<para><bold>bold</bold> <bold>several words</bold></para>
        |<para><emphasis>emphasis</emphasis> <emphasis>several words</emphasis></para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`**bold** **several words**
      |
      |*emphasis* *several words*
      `),
  );
});

test('Line break', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<para>line1<linebreak/>line2</para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`line1<br/>line2
      `),
  );
});

test('White spaces', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<para>Spaces: text<sp/>text<nonbreakablespace/>text<ensp/>text<emsp/>text<thinsp/>text</para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`Spaces: text text&nbsp;text&ensp;text&emsp;text&thinsp;text`),
  );
});

test('Hyphen and dashes', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<para>Dashes: hypen: - ndash: <ndash/> mdash: <mdash/></para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(t(`Dashes: hypen: - ndash: &ndash; mdash: &mdash;`));
});

test('Horizontal ruler', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<para><hruler/></para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(t(`---`));
});

test('Headings', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
        |<sect1 id="type_autotoc_md2">
        |<title>heading 1</title>
        |<para>text 1</para>
        |<sect2 id="type_autotoc_md3">
        |<title>heading 2</title>
        |<para>text 2</para>
        |<sect3 id="type_autotoc_md4">
        |<title>heading 3</title>
        |<para>text 3</para>
        |<sect4 id="type_autotoc_md5">
        |<title>heading 4</title>
        |<para>text 4</para>
        |<para><anchor id="type_autotoc_md6"/> <heading level="5">heading 5</heading>
        |</para>
        |<para>text 5</para>
        |<para><heading level="6">heading 6</heading>
        |</para>
        |<para>text 6</para>
        |<para><heading level="6"># heading 7</heading>
        |</para>
        |<para>text 7</para>
        |</sect4>
        |</sect3>
        |</sect2>
        |</sect1>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe(
    t(`# heading 1
      |
      |text 1
      |
      |## heading 2
      |
      |text 2
      |
      |### heading 3
      |
      |text 3
      |
      |#### heading 4
      |
      |text 4
      |
      |##### heading 5
      |
      |text 5
      |
      |###### heading 6
      |
      |text 6
      |
      |###### # heading 7
      |
      |text 7`),
  );
});

async function parse(xmlText: string) {
  const xml = await xml2js.parseStringPromise(t(xmlText), {
    explicitChildren: true,
    preserveChildrenOrder: true,
    charsAsChildren: true,
    includeWhiteChars: true,
  });
  return xml.memberdef as DoxMember;
}

function t(text: string) {
  return applyTemplateRules(text, {indent: '  ', EOL: '\n'});
}