/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

//
// The logger used by the Doxysaurus project.
//
// The goal is make the logging looking simpler in the code.
// - 'Warning:' prefixes are rendered in yellow color.
// - 'Error:' prefixes are rendered in red color.
// - Text in square brackets [] is rendered in green.
// - Text in curly brackets {} is rendered in bright cyan.
// - Repeat [] or {} brackets to render them. E.g. {{, ]], }}, ]].
// - Provide the second objTree parameter to dump a JavaScrip object.
// - Setting `log.quiet = true` disables console output.
//

import chalk from 'chalk';
import util from 'util';

export interface Logger {
  (message: string, obj?: any): void;
  quiet?: boolean;
}

export const log: Logger = (message: string, objTree?: any) => {
  const output: string[] = [];
  const warning = 'Warning:';
  const error = 'Error:';
  let index = 0;
  if (message.startsWith(warning)) {
    output.push(chalk.yellow(warning));
    index += warning.length;
  } else if (message.startsWith(error)) {
    output.push(chalk.redBright(error));
    index += error.length;
  }

  if (index === 0 && log.quiet) {
    return;
  }

  const openBrackets = '[{';
  const closeBrackets = ']}';
  let bracketIndex = -1;
  for (let i = index, ch = '\0'; (ch = message[i]); ++i) {
    if (ch === message[i + 1]) {
      if (openBrackets.includes(ch) || closeBrackets.includes(ch)) {
        output.push(message.slice(index, i));
        index = ++i;
      }
    } else if (bracketIndex >= 0 && ch === closeBrackets[bracketIndex]) {
      if (bracketIndex === 0) {
        output.push(chalk.greenBright(message.slice(index, i)));
      } else if (bracketIndex === 1) {
        output.push(chalk.cyanBright(message.slice(index, i)));
      }
      bracketIndex = -1;
      index = i + 1;
    } else {
      const tempBraceIndex = openBrackets.indexOf(ch);
      if (tempBraceIndex === -1) {
        continue;
      }

      output.push(message.slice(index, i));
      bracketIndex = tempBraceIndex;
      index = i + 1;
    }
  }
  output.push(message.slice(index, message.length));
  if (typeof objTree !== 'undefined') {
    output.push(util.inspect(objTree, {colors: true, depth: null}));
  }
  console.log(output.join(''));
};
