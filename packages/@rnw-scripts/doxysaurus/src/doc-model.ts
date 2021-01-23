/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 **/

export class DocModel {
  compounds: {[index: string]: DocCompound} = {};
}

export class DocCompound {
  namespace?: string;
  typeName?: string;
  docId?: string;
}
