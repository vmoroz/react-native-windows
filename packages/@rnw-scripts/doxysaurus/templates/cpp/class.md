---
id: {{docId}}
title: {{typeName}}
---

Defined in `{{fileName}}`  
Namespace: **`{{namespace}}`**  
{{#namespaceAlias}}
Namespace alias: **`{{.}}`**
{{/namespaceAlias}}

## Definition

```cpp
{{&prototype}}
```

{{summary}}

{{#sections}}

### {{title}}

Name | Description
-----|------------
{{#memberOverloads}}[**`{{&name}}`**]({{anchor}})| {{summary}}
{{/memberOverloads}}
{{/sections}}

{{details}}

{{#memberOverloads}}
## `{{&name}}`

{{#members}}

```cpp
{{&prototype}}
```

{{details}}
{{/members}}

{{/memberOverloads}}