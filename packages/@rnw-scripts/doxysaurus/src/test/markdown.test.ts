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
    |    <para>Test </para>
    |  </briefdescription>
    |</memberdef>`);

  const text = toMarkdown(memberDef.briefdescription);
  expect(text).toBe('Test \n\n');
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
  return applyTemplateRules(text, {indent: '  ', EOL: '\n'}).trim();
}
