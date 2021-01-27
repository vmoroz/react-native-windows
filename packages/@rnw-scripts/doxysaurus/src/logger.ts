/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import * as chalk from 'chalk';
import * as util from 'util';

export interface Logger {
  (message: string, obj?: any): void;
  quiet?: boolean;
}

export const log: Logger = (message: string, obj?: any) => {
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

  const openBraces = '[{';
  const closeBraces = ']}';
  let braceIndex = -1;
  for (let i = index, ch = '\0'; (ch = message[i]); ++i) {
    if (ch === message[i + 1]) {
      if (openBraces.includes(ch) || closeBraces.includes(ch)) {
        output.push(message.slice(index, i));
        index = ++i;
      }
    } else if (braceIndex >= 0 && ch === closeBraces[braceIndex]) {
      if (braceIndex === 0) {
        output.push(chalk.greenBright(message.slice(index, i)));
      } else if (braceIndex === 1) {
        output.push(chalk.cyanBright(message.slice(index, i)));
      }
      braceIndex = -1;
      index = i + 1;
    } else {
      const tempBraceIndex = openBraces.indexOf(ch);
      if (tempBraceIndex === -1) {
        continue;
      }

      output.push(message.slice(index, i));
      braceIndex = tempBraceIndex;
      index = i + 1;
    }
  }
  output.push(message.slice(index, message.length));
  if (typeof obj !== 'undefined') {
    output.push(util.inspect(obj, {colors: true, depth: null}));
  }
  console.log(output.join(''));
};
