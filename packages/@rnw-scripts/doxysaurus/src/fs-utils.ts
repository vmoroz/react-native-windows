/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import * as path from 'path';

// Ensures that current path is absolute.
export function ensureAbsolutePath(filePath: string, currentDir: string) {
  return path.normalize(
    path.isAbsolute(filePath) ? filePath : path.join(currentDir, filePath),
  );
}
