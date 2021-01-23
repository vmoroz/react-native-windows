/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 **/

export class DocModel {
  compounds = new Map<string, DocCompound>();
}

export class DocCompound {
  namespace?: string;
  namespaceAlias?: string;
  typeName?: string;
  docId?: string;

  prototype?: string;

  brief?: string;
  details?: string;

  sections: DocSection[] = [];
  // TODO: add templateParams
  // TODO: add support for template specializations

  baseCompounds: DocCompound[] = [];
}

export class DocSection {
  title = '';
  memberOverloads = new Map<string, DocMemberOverload>();
}

export class DocMemberOverload {
  members = new Map<string, DocMember>();
}

export class DocMember {
  parameters = new Map<string, DocParameter>();
}

export class DocParameter {}
