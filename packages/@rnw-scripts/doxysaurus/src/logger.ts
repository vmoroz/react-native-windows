/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import * as chalk from 'chalk';
import * as util from 'util';

export class Action {
  constructor(public readonly message: string) {}

  colorize(level: MessageLevel) {
    switch (level) {
      case 'verbose':
        return chalk.greenBright(this.message);
      case 'warning':
        return chalk.yellow(this.message);
      case 'error':
        return chalk.redBright(this.message);
      default:
        return this.message;
    }
  }
}

export class Arg {
  constructor(public readonly message: string) {}

  colorize() {
    return chalk.cyanBright(this.message);
  }
}

export type MessagePart = string | Action | Arg | any;

export type MessageLevel = 'verbose' | 'warning' | 'error';

// Ensures that current path is absolute.
export class Logger {
  static quiet: boolean = false;

  static verbose(...messageParts: MessagePart[]) {
    if (!Logger.quiet) {
      Logger.log('verbose', messageParts);
    }
  }

  static verboseAction(action: string, ...messageParts: MessagePart[]) {
    Logger.verbose(Logger.action(action), ...messageParts);
  }

  static warn(...messageParts: MessagePart[]) {
    Logger.log('warning', messageParts);
  }

  static error(...messageParts: MessagePart[]) {
    Logger.log('error', messageParts);
  }

  static log(level: MessageLevel, messageParts: MessagePart[]) {
    const output: string[] = [];
    for (const part of messageParts) {
      if (typeof part === 'string') {
        output.push(part);
      } else if (typeof part === 'object') {
        if (part instanceof Action) {
          output.push(part.colorize(level));
        } else if (part instanceof Arg) {
          output.push(part.colorize());
        } else {
          output.push(util.inspect(part, {colors: true, depth: null}));
        }
      } else {
        output.push(util.inspect(part, {colors: true, depth: null}));
      }
    }

    console.log(output.join(' '));
  }

  static action(arg: string) {
    return chalk.cyanBright(arg);
  }

  static arg(arg: string) {
    return chalk.cyanBright(arg);
  }
}

export function log(message: string) {
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
  console.log(output.join(''));
}
