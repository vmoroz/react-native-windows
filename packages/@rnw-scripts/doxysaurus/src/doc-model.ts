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

  brief?: string;
  details?: string;

  sections = new Map<string, DocSection>();
  // TODO: add templateParams

  baseCompounds: DocCompound[] = [];
}

export class DocSection {
  memberOverloads = new Map<string, DocMemberOverload>();
}

export class DocMemberOverload {
  members = new Map<string, DocMember>();
}

export class DocMember {
  parameters = new Map<string, DocParameter>();
}

export class DocParameter {}
