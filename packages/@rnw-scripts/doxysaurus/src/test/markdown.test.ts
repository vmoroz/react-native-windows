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
// The real Doxygen XML often omits any spaces between tags. Here we use extra
// spaces in cases when they are omitted by XML parser to indent XML for readability.
//
// These tests are using the string-template to have more natural string look and feel.
// The leading pipeline means a beginning of the line.
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

test('Brief description with a paragraph', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
    |    <para>Test</para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe('Test');
});

test('Brief description with three paragraphs', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
    |    <para>Paragraph1</para>
    |    <para>Paragraph2</para>
    |    <para>Paragraph3</para>
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
    |    <para>
    |      <itemizedlist>
    |        <listitem>
    |          <para>item1</para>
    |        </listitem>
    |        <listitem>
    |          <para>item2</para>
    |        </listitem>
    |        <listitem>
    |          <para>item3</para>
    |        </listitem>
    |      </itemizedlist>
    |    </para>
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

test('Brief description with paragraphs in itemized list', async () => {
  const memberDef = await parse(`
  |<memberdef>
  |  <briefdescription>
  |   <para>
  |     <itemizedlist>
  |       <listitem>
  |         <para>item1 para1</para>
  |         <para>item1 para2</para>
  |         <para>item1 para3</para>
  |       </listitem>
  |       <listitem>
  |         <para>item2</para>
  |       </listitem>
  |       <listitem>
  |         <para>item3</para>
  |       </listitem>
  |     </itemizedlist>
  |   </para>
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

test('Brief description with ordered list', async () => {
  const memberDef = await parse(`
  |<memberdef>
  |  <briefdescription>
  |    <para>
  |      <orderedlist>
  |        <listitem>
  |          <para>item1</para>
  |        </listitem>
  |        <listitem>
  |          <para>item2</para>
  |        </listitem>
  |        <listitem>
  |          <para>item3</para>
  |        </listitem>
  |      </orderedlist>
  |    </para>
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

test('Brief description with paragraphs in ordered list', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
    |   <para>
    |     <orderedlist>
    |       <listitem>
    |         <para>item1 para1</para>
    |         <para>item1 para2</para>
    |         <para>item1 para3</para>
    |       </listitem>
    |       <listitem>
    |         <para>item2</para>
    |       </listitem>
    |       <listitem>
    |         <para>item3</para>
    |       </listitem>
    |     </orderedlist>
    |   </para>
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

test('Brief description with nested ordered list', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
    |   <para>
    |     <orderedlist>
    |       <listitem>
    |         <para>item1<orderedlist>
    |           <listitem>
    |             <para>item1 item1</para>
    |           </listitem>
    |           <listitem>
    |             <para>item1 item2</para>
    |           </listitem>
    |         </orderedlist></para>
    |       </listitem>
    |       <listitem>
    |         <para>item2<itemizedlist>
    |           <listitem>
    |             <para>item2 item1</para>
    |           </listitem>
    |           <listitem>
    |             <para>item2 item2</para>
    |           </listitem>
    |         </itemizedlist></para>
    |       </listitem>
    |       <listitem>
    |         <para>item3</para>
    |       </listitem>
    |     </orderedlist>
    |   </para>
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

test('Brief description with nested itemized list', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
    |   <para>
    |     <itemizedlist>
    |       <listitem>
    |         <para>item1<orderedlist>
    |           <listitem>
    |             <para>item1 item1</para>
    |           </listitem>
    |           <listitem>
    |             <para>item1 item2</para>
    |           </listitem>
    |         </orderedlist></para>
    |       </listitem>
    |       <listitem>
    |         <para>item2<itemizedlist>
    |           <listitem>
    |             <para>item2 item1</para>
    |           </listitem>
    |           <listitem>
    |             <para>item2 item2</para>
    |           </listitem>
    |         </itemizedlist></para>
    |       </listitem>
    |       <listitem>
    |         <para>item3</para>
    |       </listitem>
    |     </itemizedlist>
    |   </para>
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

test('Brief description with itemized list between paragraphs', async () => {
  const memberDef = await parse(`
    |<memberdef>
    |  <briefdescription>
    |   <para>text1</para>
    |   <para>
    |     <itemizedlist>
    |       <listitem>
    |         <para>item1</para>
    |       </listitem>
    |       <listitem>
    |         <para>item2</para>
    |       </listitem>
    |       <listitem>
    |         <para>item3</para>
    |       </listitem>
    |     </itemizedlist>
    |   </para>
    |   <para>text2</para>
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
