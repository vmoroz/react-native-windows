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
  namespaceAlias?: string;
  typeName?: string;
  docId?: string;

  prototype?: string;

  brief: string = '';
  details: string = '';
  summary: string = ''; // Assembled from and brief and details

  sections: DocSection[] = [];
  memberOverloads: DocMemberOverload[] = [];
  // TODO: add templateParams
  // TODO: add support for template specializations

  baseCompounds: DocCompound[] = [];
  fileName = '';
  output = '';
}

export class DocSection {
  title = '';
  memberOverloads: DocMemberOverload[] = [];
  line = 0;
}

export class DocMemberOverload {
  name = '';
  members: DocMember[] = [];
  anchor = '#';
  summary = ''; // First member summary
  line = 0;

  constructor(public compound: DocCompound) {}

  static compareByName(
    left: DocMemberOverload,
    right: DocMemberOverload,
  ): number {
    if (left.name < right.name) {
      return -1;
    } else if (right.name > left.name) {
      return 1;
    } else {
      return 0;
    }
  }
}

export class DocMember {
  name = '';
  overload?: DocMemberOverload;
  brief: string = '';
  details: string = '';
  summary: string = ''; // Assembled from and brief and details
  prototype = '';
  line = 0;
}
